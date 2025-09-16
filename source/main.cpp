#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/stdio_uart.h"
#include "pico/multicore.h"
#include "tusb_config.h"
#include "tusb.h"
#include "hardware/pwm.h"
#include "ringbuffer.h"
#include "SquareWave.hpp"
#include "NoiseDrum.hpp"
#include <cstdio>

// GPIO-00 UART-0 TX
// GPIO-01 UART-0 RX
// GPIO-06 Audio out

static const int PWM_PIN = 6;
static const int PSG_DEVIDE_FACTOR = 9;
static const int CHANNEL_COUNT = 32;
static const int NOISE_DRUM_COUNT = 11;
constexpr double CLOCK_FREQ = 125000000.0;

// リズムノート変換テーブル
static const uint8_t Note35_57ChangeTable[] =
{
	0 , 0, 5, 1, 255, 6, 2, 7,
	4, 255, 2, 8, 3, 3, 9, 4,
	10, 255, 255, 255, 255, 255, 9
};

uint16_t psg_master_volume;
uint8_t midi_ch_volume[16];
SquareWave squareWave[CHANNEL_COUNT];

NoiseDrum noiseDrum[NOISE_DRUM_COUNT];

repeating_timer timer;
bool callbackBusy = false;

void setup_pwm()
{
	gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
	uint slice = pwm_gpio_to_slice_num(PWM_PIN);
	pwm_set_wrap(slice, 255); // 8bit resolution
	pwm_set_clkdiv(slice, CLOCK_FREQ / (SquareWave::OUTPUT_SAMPLING_FREQUENCY * 256.0));
	pwm_set_enabled(slice, true);
}

// タイマ割り込み処理
bool timerCallback(repeating_timer *t)
{
	if(callbackBusy == true)
	{
		return true;
	}
	callbackBusy = true;
	uint16_t mix_volume;
	// Mixer
	mix_volume = 0;
	for(int i = 0; i < CHANNEL_COUNT; i ++)
	{
		uint8_t wave = squareWave[i].GetData();
		if(squareWave[i].psg_tone_on == 1)
		{
			if(wave != 0)
			{
				mix_volume += SquareWave::psgVolume[midi_ch_volume[squareWave[i].psg_midi_inuse_ch] * 2 + 1];
			}
		}
	}
	for(int i = 0; i < NOISE_DRUM_COUNT; ++ i)
	{
		mix_volume += noiseDrum[i].GetData();
	}
	psg_master_volume = mix_volume / PSG_DEVIDE_FACTOR;
	if(psg_master_volume > 255)
	{
		psg_master_volume = 255;
	}
	pwm_set_gpio_level(PWM_PIN, psg_master_volume);
	callbackBusy = false;
	return true;
}

void setup_timer()
{
	add_repeating_timer_us(-1000000 / SquareWave::OUTPUT_SAMPLING_FREQUENCY, timerCallback, NULL, &timer);
}

// Core1の処理
void core1_entry()
{
	uint8_t rxData;
	uint8_t midicc1;
	uint8_t midicc2;
	uint8_t midinote;
	uint8_t midivel;
	uint8_t override;
	setup_pwm();
	setup_timer();
	while(true)
	{
		if(rb_count() < 1)
		{
			tight_loop_contents();
		}
		if(!rb_pop(&rxData))
		{
			tight_loop_contents();
			continue;
		}
		// データを受信した
		// Listen USART
		uint8_t midicmd = rxData;
		uint8_t midich = midicmd & 0xF;
		switch(midicmd & 0xF0)
		{
		case 0x80: // Note off
			while(rb_count() < 2)
			{
				tight_loop_contents();
			}
			rb_pop(&midinote);
			rb_pop(&midivel);
			for(int i = 0; i < CHANNEL_COUNT; ++ i)
			{
				if((squareWave[i].psg_midi_inuse == 1) && (squareWave[i].psg_midi_inuse_ch == midich) && (squareWave[i].psg_midi_note == midinote))
				{
					squareWave[i].NoteOff(midinote);
					squareWave[i].psg_midi_inuse = 0;
				}
			}
			break;
		case 0x90: // Note on
			while(rb_count() < 2)
			{
				tight_loop_contents();
			}
			rb_pop(&midinote);
			rb_pop(&midivel);

//			printf("midicmd: %02X %02X %02X\n", midicmd, midinote, midivel);

			if(midich != 9)
			{
				if(midivel != 0)
				{
					// check note is already on
					override = 0;
					for(int i = 0; i < CHANNEL_COUNT; ++ i)
					{
						if((squareWave[i].psg_midi_inuse == 1) && (squareWave[i].psg_midi_inuse_ch == midich) && (squareWave[i].psg_midi_note == midinote))
						{
							override=1;
						}
					}
					if(override == 0)
					{
						for(int i = 0; i < CHANNEL_COUNT; ++ i)
						{
							if(squareWave[i].psg_midi_inuse == 0)
							{
								squareWave[i].NoteOn(midinote,midi_ch_volume[midich]);
								squareWave[i].psg_midi_inuse = 1;
								squareWave[i].psg_midi_inuse_ch = midich;
								squareWave[i].psg_midi_note = midinote;
								break;
							}
						}
					}
				} else {
					for(int i = 0; i < CHANNEL_COUNT; ++ i)
					{
						if((squareWave[i].psg_midi_inuse == 1) && (squareWave[i].psg_midi_inuse_ch == midich) && (squareWave[i].psg_midi_note == midinote))
						{
							squareWave[i].NoteOff(midinote);
							squareWave[i].psg_midi_inuse = 0;
						}
					}
				}
			}
			else
			{
				if((35 <= midinote) && (midinote <= 57))
				{
					uint8_t rythmNote = Note35_57ChangeTable[midinote - 35];
					if(rythmNote < 11)
					{
						noiseDrum[rythmNote].SetPlay(rythmNote);
					}
				}
			}
			break;
		case 0xB0:
			// Channel control
			while(rb_count() < 1)
			{
				tight_loop_contents();
			}
			rb_pop(&midicc1);
			switch(midicc1)
			{
			case 7:
			case 11: // Expression
			while(rb_count() < 1)
			{
				tight_loop_contents();
			}
				rb_pop(&midicc2);
				midi_ch_volume[midich] = (midicc2 >> 3);
				if(midich == 9)
				{
					for(int i = 0; i < NOISE_DRUM_COUNT; ++ i)
					{
						noiseDrum[i].SetVolume(midi_ch_volume[midich]);
					}
				}
				break;
			case 0: //Bank select
			case 120:// All note off
			case 121:// All reset
			case 123:
			case 124:
			case 125:
			case 126:
			case 127:
				for(int i = 0; i < CHANNEL_COUNT; ++ i)
				{
					if((squareWave[i].psg_midi_inuse == 1) && (squareWave[i].psg_midi_inuse_ch == midich))
					{
						squareWave[i].NoteOff(squareWave[i].psg_midi_note);
						squareWave[i].psg_midi_inuse = 0;
					}
				}
				break;
			default:
				break;
			}
			break;
		case 0xC0:
			// Program change
			for(int i = 0; i < CHANNEL_COUNT; ++ i)
			{
				if((squareWave[i].psg_midi_inuse == 1) && (squareWave[i].psg_midi_inuse_ch == midich))
				{
					squareWave[i].NoteOff(squareWave[i].psg_midi_note);
					squareWave[i].psg_midi_inuse = 0;
				}
			}
			break;
		default: // Skip
			break;
		}
		tight_loop_contents();
	}
}

// Core0の処理
int main()
{
	stdio_init_all();
	stdio_uart_init();

	// core1設定
	multicore_launch_core1(core1_entry);

	// UART初期化
	uart_init(uart0, 9600);
	gpio_set_function(12, GPIO_FUNC_UART);  // TX = GP0
	gpio_set_function(13, GPIO_FUNC_UART);  // RX = GP1

	printf("PicoMidi start.\n");

	// USBスタック初期化
	tusb_init();
	uint8_t buffer[64];
	int phase = 0;
	while(true)
	{
		tud_task();	// USBイベント処理
		if(tud_midi_available())
		{
			uint32_t count = tud_midi_stream_read(buffer, sizeof(buffer));
			for(uint32_t i = 0; i < count; ++ i)
			{
				rb_push(buffer[i]);
			}
		}
	}
}
