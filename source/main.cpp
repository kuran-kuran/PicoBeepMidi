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
#include "SinWave.hpp"
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

uint16_t masterVolume;
uint8_t midiChannelVolume[16] = {
	254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254
};
uint8_t midiChannelExpression[16] = {
	254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254
};
SquareWave squareWave[CHANNEL_COUNT];

NoiseDrum noiseDrum[NOISE_DRUM_COUNT];

repeating_timer timer;
volatile bool callbackBusy = false;

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
	// Mixer
	uint16_t mix_volume = 0;
	for(int i = 0; i < CHANNEL_COUNT; i ++)
	{
		int channel = squareWave[i].GetChannel();
		mix_volume += squareWave[i].GetData(static_cast<int32_t>(midiChannelVolume[channel] * midiChannelExpression[channel] >> 8));
	}
	for(int i = 0; i < NOISE_DRUM_COUNT; ++ i)
	{
		mix_volume += noiseDrum[i].GetData();
	}
	masterVolume = mix_volume / PSG_DEVIDE_FACTOR;
	if(masterVolume > 255)
	{
		masterVolume = 255;
	}
	pwm_set_gpio_level(PWM_PIN, masterVolume);
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
//			printf("midicmd: (%d) %02X %02X %02X\n", midich, midicmd, midinote, midivel);
			for(int i = 0; i < CHANNEL_COUNT; ++ i)
			{
				if(squareWave[i].IsInUse() && (squareWave[i].GetChannel() == midich) && (squareWave[i].GetNote() == midinote))
				{
					squareWave[i].NoteOff();
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
//			printf("midicmd: (%d) %02X %02X %02X v(%d)\n", midich, midicmd, midinote, midivel, midiChannelVolume[midich]);
			if(midich != 9)
			{
				if(midivel != 0)
				{
					// check note is already on
					override = 0;
					for(int i = 0; i < CHANNEL_COUNT; ++ i)
					{
						if(squareWave[i].IsInUse() && (squareWave[i].GetChannel() == midich) && (squareWave[i].GetNote() == midinote))
						{
							override = 1;
						}
					}
					if(override == 0)
					{
						for(int i = 0; i < CHANNEL_COUNT; ++ i)
						{
							if(!squareWave[i].IsInUse())
							{
								squareWave[i].NoteOn(midinote, midivel << 1);
								squareWave[i].SetChannel(midich);
								break;
							}
						}
					}
				}
				else
				{
					for(int i = 0; i < CHANNEL_COUNT; ++ i)
					{
						if(squareWave[i].IsInUse() && (squareWave[i].GetChannel() == midich) && (squareWave[i].GetNote() == midinote))
						{
							squareWave[i].NoteOff();
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
						noiseDrum[rythmNote].SetPlay(rythmNote, midivel >> 3);
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
//			printf("midicmd: (%d) %02X %02X\n", midich, midicmd, midicc1);
			switch(midicc1)
			{
			case 7:  // Volume
			case 11: // Expression
				while(rb_count() < 1)
				{
					tight_loop_contents();
				}
				rb_pop(&midicc2);
//				printf("*midicmd: (%d) %02X %02X %02X\n", midich, midicmd, midicc1, midicc2);
				if(midicc1 == 7)
				{
					midiChannelVolume[midich] = midicc2 << 1; // (0～254)
				}
				else
				{
					midiChannelExpression[midich] = midicc2 << 1; // (0～254)
				}
				if(midich == 9)
				{
					for(int i = 0; i < NOISE_DRUM_COUNT; ++ i)
					{
						uint8_t setVolume = static_cast<uint8_t>((midiChannelVolume[midich] * midiChannelExpression[midich]) >> 12);
						noiseDrum[i].SetVolume(setVolume);
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
					if(squareWave[i].IsInUse() && (squareWave[i].GetChannel() == midich))
					{
						squareWave[i].NoteOff();
					}
				}
				break;
			default:
				break;
			}
			break;
		case 0xC0:
			// Program change
//			printf("Program change: (%d) %02X\n", midich, midicc2);
			for(int i = 0; i < CHANNEL_COUNT; ++ i)
			{
				if(squareWave[i].IsInUse() && (squareWave[i].GetChannel() == midich))
				{
					squareWave[i].NoteOff();
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
	uart_init(uart0, 31250); // MIDI: 31250, PC:38400
	gpio_set_function(12, GPIO_FUNC_UART);  // TX = GP12
	gpio_set_function(13, GPIO_FUNC_UART);  // RX = GP13

//	printf("PicoMidi start.\n");

	// USBスタック初期化
	tusb_init();
	uint8_t buffer[64];
	int phase = 0;
	while(true)
	{
		// USBイベント処理
		tud_task();
		// USB MIDI受信
		if(tud_midi_available())
		{
			uint32_t count = tud_midi_stream_read(buffer, sizeof(buffer));
			for(uint32_t i = 0; i < count; ++ i)
			{
				rb_push(buffer[i]);
			}
		}
		// UART受信
		while(uart_is_readable(uart0))
		{
			uint8_t rxData = uart_getc(uart0);
			// FIFO内の全データをここで処理
			rb_push(rxData);
		}
		tight_loop_contents();
	}
}
