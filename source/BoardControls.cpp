#include "BoardConfig.h"
#include "BoardControls.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#if MODE_LED_RGB
#include "mode_led.pio.h"
#endif
#include <atomic>

uint32_t board_button_timeouts = 0;
uint32_t board_max_button_us = 0;

namespace {
std::atomic<uint32_t> requested_pause{0}, acknowledged_pause{0};
uint32_t pause_sequence = 0; // Written only by core 1.
#if MODE_LED_PIN >= 0 && MODE_LED_RGB
uint led_sm;
#endif

// No flash instruction/data fetch or interrupt is allowed while CS is floated.
// Core 0 has acknowledged that it is parked with interrupts disabled in SRAM.
bool __no_inline_not_in_flash_func(read_bootsel_sram)()
{
    const uint32_t irq = save_and_disable_interrupts();
    const uint32_t saved = ioqspi_hw->io[1].ctrl;
    ioqspi_hw->io[1].ctrl = (saved & ~IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS) |
        (GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB);
    for(volatile unsigned n = 0; n < 1000; ++n) {}
    const bool down = !(sio_hw->gpio_hi_in & (1u<<1));
    ioqspi_hw->io[1].ctrl = saved;
    restore_interrupts(irq);
    return down;
}
}

void board_controls_init()
{
#if MODE_LED_PIN >= 0
    static_assert(MODE_LED_PIN != AUDIO_PWM_PIN && MODE_LED_PIN != (AUDIO_PWM_PIN^1), "LED overlaps audio PWM");
#if MODE_LED_RGB
    led_sm = static_cast<uint>(pio_claim_unused_sm(pio0,true));
    const uint offset = pio_add_program(pio0,&mode_led_program);
    pio_gpio_init(pio0,MODE_LED_PIN);
    pio_sm_set_consecutive_pindirs(pio0,led_sm,MODE_LED_PIN,1,true);
    auto config = mode_led_program_get_default_config(offset);
    sm_config_set_sideset_pins(&config,MODE_LED_PIN);
    sm_config_set_out_shift(&config,false,true,24);
    sm_config_set_fifo_join(&config,PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&config,static_cast<float>(clock_get_hz(clk_sys))/8000000.0f);
    pio_sm_init(pio0,led_sm,offset,&config);
    pio_sm_set_enabled(pio0,led_sm,true);
#else
    // GPIOs 16 apart share a PWM slice on RP2040; keep LED and audio independent.
    static_assert(((MODE_LED_PIN >> 1) & 7) != ((AUDIO_PWM_PIN >> 1) & 7), "LED shares audio PWM slice");
    gpio_set_function(MODE_LED_PIN,GPIO_FUNC_PWM);
    auto pwm = pwm_get_default_config();
    pwm_config_set_wrap(&pwm,999);
    pwm_config_set_clkdiv(&pwm,static_cast<float>(clock_get_hz(clk_sys))/1000000.0f); // 1 kHz.
    pwm_set_gpio_level(MODE_LED_PIN,1000);
    pwm_init(pwm_gpio_to_slice_num(MODE_LED_PIN),&pwm,true);
#endif
#endif
    board_mode_led(false);
}

void board_mode_led(bool beep)
{
#if MODE_LED_PIN >= 0
#if MODE_LED_RGB
    // WS2812 expects GRB, MSB first. Use low brightness for a status indicator.
    const uint32_t grb = beep ? 0x001000u : 0x100000u;
    pio_sm_put_blocking(pio0,led_sm,grb<<8);
#else
    // Hardware PWM: GM 100%, BEEP 10%; no interrupt or audio-loop overhead.
    pwm_set_gpio_level(MODE_LED_PIN,beep ? 100 : 1000);
#endif
#else
    (void)beep;
#endif
}

// Called only between core 0's USB/UART work, outside queues or SDK locks.
// All instructions, literal constants and shared state here must reside in RAM.
void __no_inline_not_in_flash_func(board_controls_service)()
{
    const uint32_t request = requested_pause.load(std::memory_order_acquire);
    if(!request) return;
    const uint32_t irq = save_and_disable_interrupts();
    acknowledged_pause.store(request,std::memory_order_release);
    while(requested_pause.load(std::memory_order_acquire) == request) {
        // No SDK locks, flash access, interrupt handlers or FIFO handshake.
        __asm volatile("nop");
    }
    acknowledged_pause.store(0,std::memory_order_release);
    restore_interrupts(irq);
}

bool board_bootsel_read(bool& down)
{
    const uint32_t started = time_us_32();
    // Do not mistake the acknowledgement of a canceled request for a new one.
    if(acknowledged_pause.load(std::memory_order_acquire)) {
        ++board_button_timeouts;
        return false;
    }
    if(++pause_sequence == 0) ++pause_sequence;
    const uint32_t request = pause_sequence;
    requested_pause.store(request,std::memory_order_release);
    bool obtained = false;
    while(static_cast<uint32_t>(time_us_32()-started) < 200) {
        if(acknowledged_pause.load(std::memory_order_acquire) == request) {
            down = read_bootsel_sram();
            obtained = true;
            break;
        }
    }
    // Release on both success and timeout; flash CS is restored first.
    requested_pause.store(0,std::memory_order_release);
    const uint32_t elapsed = time_us_32()-started;
    if(elapsed > board_max_button_us) board_max_button_us = elapsed;
    if(!obtained) ++board_button_timeouts;
    return obtained;
}

bool board_flash_operation(void (*operation)(void*), void* context)
{
    if(acknowledged_pause.load(std::memory_order_acquire)) return false;
    if(++pause_sequence == 0) ++pause_sequence;
    const uint32_t request = pause_sequence;
    const uint32_t started = time_us_32();
    requested_pause.store(request,std::memory_order_release);
    bool obtained = false;
    while(static_cast<uint32_t>(time_us_32()-started) < 200) {
        if(acknowledged_pause.load(std::memory_order_acquire) == request) {
            operation(context);
            obtained = true;
            break;
        }
    }
    requested_pause.store(0,std::memory_order_release);
    return obtained;
}
