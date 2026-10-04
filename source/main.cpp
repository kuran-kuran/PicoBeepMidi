#include "BoardConfig.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "tusb.h"
#include "Synth.h"
#include "BoardControls.h"
#include "ModeButton.h"
#include "AudioBuffers.h"
#include "ModeStorage.h"
#include <atomic>

#define MIDI_UART_RX_PIN 1
#define MIDI_UART_TX_PIN 0

static_assert(MODE_LED_PIN != MIDI_UART_RX_PIN && MODE_LED_PIN != MIDI_UART_TX_PIN, "LED overlaps MIDI UART");

namespace {
queue_t messages;
queue_t diagnostics;
std::atomic<bool> diagnostics_requested{false};
struct Diagnostics { uint32_t values[14]; };
std::atomic<uint32_t> panic_epoch{0};
// Debugger-visible diagnostics; one writer (audio core) per field.
volatile uint32_t audio_underruns = 0;
volatile uint32_t max_render_us = 0;
volatile uint32_t event_overflows = 0;
uint32_t uart_errors = 0;
constexpr uint32_t block_size = 128;

struct AudioDma {
    uint channel;
    bool busy() const { return dma_channel_is_busy(channel); }
    void idle() const { tight_loop_contents(); }
    void start(const uint32_t* samples, unsigned count) const
    {
        hard_assert(!busy());
        dma_channel_set_read_addr(channel,samples,false);
        dma_channel_set_trans_count(channel,count,true);
    }
};

void panic()
{
    panic_epoch.store(panic_epoch.load(std::memory_order_relaxed)+1,std::memory_order_release);
}

class Receiver : public MidiSink {
    void receive(const MidiMessage& message) override
    {
        if(message.status == 0xf0 && message.size == 4 &&
           message.data[0] == 0x7d && message.data[1] == 0x47 &&
           message.data[2] == 0x4d && message.data[3] == 1) {
            diagnostics_requested.store(true,std::memory_order_release);
            return;
        }
        if(!queue_try_add(&messages,&message)) { ++event_overflows; panic(); }
    }
};
Receiver receiver;
MidiParser usb_parser(receiver), uart_parser(receiver);

uint32_t gcd(uint32_t a, uint32_t b)
{
    while(b) { uint32_t t = a%b; a = b; b = t; }
    return a;
}

void audio_core()
{
    static Synth synth;
    board_controls_init();
    uint8_t saved_mode = mode_storage_load();
    synth.set_mode(saved_mode == 2 ? Synth::Mode::beep : Synth::Mode::gm);
    board_mode_led(saved_mode == 2);
    uint32_t mode_changed_at = 0, save_attempt_at = 0;
    ModeButton mode_button;
    uint32_t last_button = 0;
    gpio_set_function(AUDIO_PWM_PIN,GPIO_FUNC_PWM);
    const uint slice = pwm_gpio_to_slice_num(AUDIO_PWM_PIN);
    const uint shift = pwm_gpio_to_channel(AUDIO_PWM_PIN)*16;
    pwm_config pwm = pwm_get_default_config();
    // Preserve the 125 MHz build's PWM carrier when testing a faster CPU.
    pwm_config_set_clkdiv(&pwm,static_cast<float>(clock_get_hz(clk_sys))/125000000.0f);
    pwm_config_set_wrap(&pwm,255);
    pwm_init(slice,&pwm,true);
    pwm_set_gpio_level(AUDIO_PWM_PIN,128);
    const int timer = dma_claim_unused_timer(true);
    const uint32_t clock = clock_get_hz(clk_sys), divisor = gcd(clock,Synth::sample_rate);
    // Default 125 MHz gives 6/15625: exact 48 kHz pacing, independent of PWM carrier.
    hard_assert(clock/divisor <= 65535 && Synth::sample_rate/divisor <= 65535);
    dma_timer_set_fraction(static_cast<uint>(timer),static_cast<uint16_t>(Synth::sample_rate/divisor),static_cast<uint16_t>(clock/divisor));
    const uint dma_channel = static_cast<uint>(dma_claim_unused_channel(true));
    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&c,DMA_SIZE_32);
    channel_config_set_read_increment(&c,true);
    channel_config_set_write_increment(&c,false);
    channel_config_set_dreq(&c,dma_get_timer_dreq(static_cast<uint>(timer)));
    channel_config_set_chain_to(&c,dma_channel); // Self means no chaining.
    dma_channel_configure(dma_channel,&c,&pwm_hw->slice[slice].cc,nullptr,0,false);
    AudioDma transfer{dma_channel};
    // Static storage: keep these 1 KiB buffers off core 1's small stack.
    static AudioBuffers<AudioDma,block_size> audio(transfer);
    static int16_t pcm[block_size];
    audio.start(128u<<shift);
    uint32_t seen_panic = 0;
    while(true) {
        const uint32_t started = time_us_32();
        if(static_cast<uint32_t>(started-last_button) >= 10000) {
            last_button = started;
            bool down = false;
            const bool valid = board_bootsel_read(down);
            const auto action = valid ? mode_button.update(down,time_us_32()) : ModeButton::Action::none;
            if(!valid) mode_button = ModeButton{}; // Require release after a missed reading.
            if(action == ModeButton::Action::toggle_mode) {
                synth.set_mode(synth.mode() == Synth::Mode::gm ? Synth::Mode::beep : Synth::Mode::gm);
                board_mode_led(synth.mode() == Synth::Mode::beep);
                mode_changed_at = time_us_32();
            }
            if(action == ModeButton::Action::reset) synth.reset();
        }
        const uint32_t epoch = panic_epoch.load(std::memory_order_acquire);
        MidiMessage m;
        if(epoch != seen_panic) {
            while(queue_try_remove(&messages,&m)) {}
            synth.all_sound_off(); seen_panic = epoch;
        }
        // Bound control work so a USB flood cannot starve rendering indefinitely.
        for(unsigned n = 0; n < 32 && queue_try_remove(&messages,&m); ++n) synth.receive(m);
        const uint8_t current_mode = synth.mode() == Synth::Mode::beep ? 2 : 1;
        const uint32_t now = time_us_32();
        if(current_mode != saved_mode && now-mode_changed_at >= 2000000u &&
           now-save_attempt_at >= 1000000u && synth.active_count() == 0 && queue_is_empty(&messages)) {
            save_attempt_at = now;
            // Finish queued audio before parking both CPUs for flash programming.
            while(transfer.busy()) transfer.idle();
            pwm_set_gpio_level(AUDIO_PWM_PIN,128);
            if(mode_storage_save(current_mode)) saved_mode = current_mode;
            audio.start(128u<<shift);
            continue;
        }
        uint32_t* const buffer = audio.writable();
        synth.render_block(pcm,block_size);
        for(unsigned n = 0; n < block_size; ++n) {
            const int32_t sample = pcm[n];
            buffer[n] = static_cast<uint32_t>((sample+32768)>>8)<<shift;
        }
        const uint32_t elapsed = time_us_32()-started;
        if(elapsed > max_render_us) max_render_us = elapsed;
        if(audio.submit()) ++audio_underruns;
        if(diagnostics_requested.load(std::memory_order_acquire)) {
            diagnostics_requested.store(false,std::memory_order_release);
            const Diagnostics d{{5,audio_underruns,max_render_us,0,
                synth.note_ons,synth.note_offs,synth.unmatched_offs,
                synth.active_count(),synth.held_count(),synth.voice_steals,
                board_button_timeouts,board_max_button_us,0,0}};
            queue_try_add(&diagnostics,&d);
        }
    }
}
} // namespace

int main()
{
    // Set voltage/clock before UART, USB, PIO or the audio core are initialized.
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(10);
    set_sys_clock_khz(250000,true);
    queue_init(&messages,sizeof(MidiMessage),256);
    queue_init(&diagnostics,sizeof(Diagnostics),1);
    uart_init(uart0,31250);
    gpio_set_function(MIDI_UART_RX_PIN,GPIO_FUNC_UART);
    gpio_pull_up(MIDI_UART_RX_PIN); // Keep an unused UART idle during USB-only use.
    gpio_set_function(MIDI_UART_TX_PIN,GPIO_FUNC_UART);
    uart_set_format(uart0,8,1,UART_PARITY_NONE);
    uart_set_fifo_enabled(uart0,true);
    tusb_init();
    multicore_launch_core1(audio_core);
    bool was_mounted = false;
    bool sensing[2] = {false,false};
    uint32_t last_rx[2] = {0,0};
    auto feed = [&](unsigned source, uint8_t byte) {
        last_rx[source] = time_us_32();
        if(byte == 0xfe) sensing[source] = true;
        (source == 0 ? usb_parser : uart_parser).feed(byte);
    };
    while(true) {
        board_controls_service();
        tud_task();
        static uint8_t reply[76];
        static uint32_t reply_offset = 0, reply_size = 0;
        Diagnostics d;
        if(reply_offset == reply_size && queue_try_remove(&diagnostics,&d)) {
            d.values[3] = event_overflows;
            d.values[12] = uart_errors;
            d.values[13] = panic_epoch.load(std::memory_order_relaxed);
            reply[0] = 0xf0; reply[1] = 0x7d; reply[2] = 0x47; reply[3] = 0x4d; reply[4] = 2;
            for(unsigned i = 0; i < 14; ++i)
                for(unsigned j = 0; j < 5; ++j) reply[5+i*5+j] = (d.values[i]>>(7*j))&127;
            reply[75] = 0xf7; reply_offset = 0; reply_size = sizeof(reply);
        }
        if(reply_offset < reply_size && tud_mounted())
            reply_offset += tud_midi_stream_write(0,reply+reply_offset,reply_size-reply_offset);
        const bool mounted = tud_mounted();
        if(was_mounted && !mounted) { usb_parser.reset(); sensing[0] = false; panic(); }
        was_mounted = mounted;
        uint8_t packet[4];
        static const uint8_t cin_size[16] = {0,0,2,3,3,1,2,3,3,3,3,3,2,2,3,1};
        // Separate USB cable 0 and UART parser state prevents running-status corruption.
        for(unsigned n = 0; n < 32 && tud_midi_packet_read(packet); ++n) {
            if((packet[0]>>4) != 0) continue;
            for(uint8_t i = 0; i < cin_size[packet[0]&15]; ++i) feed(0,packet[i+1]);
        }
        for(unsigned n = 0; n < 32 && uart_is_readable(uart0); ++n) {
            const uint32_t data = uart_get_hw(uart0)->dr;
            if(data & 0x0f00u) {
                ++uart_errors;
                // Framing/parity/break/overrun: never carry a damaged running status forward.
                uart_get_hw(uart0)->rsr = 0;
                uart_parser.reset(); panic();
            } else feed(1,static_cast<uint8_t>(data));
        }
        for(unsigned source = 0; source < 2; ++source) {
            if(sensing[source] && time_us_32()-last_rx[source] > 300000) {
                sensing[source] = false;
                (source == 0 ? usb_parser : uart_parser).reset(); panic();
            }
        }
        tight_loop_contents();
    }
}
