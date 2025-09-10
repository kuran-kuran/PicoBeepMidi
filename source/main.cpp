#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/stdio_uart.h"
#include "pico/multicore.h"
#include "tusb_config.h"
#include "tusb.h"
#include "ringbuffer.h"
#include "SquareWave.hpp"
#include "NoiseDrum.hpp"
#include <cstdio>

// GPIO0 UART0 TX
// GPIO1 UART0 RX

// リズムノート変換テーブル
static const uint8_t Note35_57ChangeTable[] =
{
	0 , 0, 5, 1, 255, 6, 2, 7,
	4, 255, 2, 8, 3, 3, 9, 4,
	10, 255, 255, 255, 255, 255, 9
};

uint16_t psg_master_volume;
uint8_t midi_ch_volume[16];

/*
// タイマ割り込み処理
void SysTick_Handler(void)
{
	uint16_t master_volume;
	uint8_t tone_output[CHANNEL_COUNT];
	TIM1->CH4CVR = psg_master_volume;
// Run Oscillator
	for(int i = 0; i < CHANNEL_COUNT; i ++)
	{
		uint32_t pon_count = beep[i].psg_osc_counter += SAMPLING_INTERVAL;
		if(pon_count < (beep[i].psg_osc_intervalHalf))
		{
			tone_output[i] = beep[i].psg_tone_on;
		}
		else if (pon_count > beep[i].psg_osc_interval)
		{
			beep[i].psg_osc_counter -= beep[i].psg_osc_interval;
			tone_output[i] = beep[i].psg_tone_on;
		}
		else
		{
			tone_output[i] = 0;
		}
	}
// Mixer
	master_volume = 0;
	for(int i = 0; i < CHANNEL_COUNT; i ++)
	{
		if(beep[i].psg_tone_on == 1)
		{
			if(tone_output[i] != 0)
			{
				master_volume += psg_volume[midi_ch_volume[beep[i].psg_midi_inuse_ch] * 2 + 1];
			}
		}
	}
	master_volume += NoiseDrumGetData(&drum);
	psg_master_volume = master_volume / PSG_DEVIDE_FACTOR;
	if(psg_master_volume > 255)
	{
		psg_master_volume = 255;
	}
	SysTick->SR &= 0;
}
*/

// Core1の処理
void core1_entry()
{
/*
	uint8_t rxData;
	uint8_t midinote;
	while(true)
	{
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
			midinote = ReadByte();
			midivel = ReadByte();
			for(int i = 0; i < CHANNEL_COUNT; ++ i)
			{
				if((beep[i].psg_midi_inuse == 1) && (beep[i].psg_midi_inuse_ch == midich) && (beep[i].psg_midi_note == midinote))
				{
					noteoff(i, midinote);
					beep[i].psg_midi_inuse = 0;
				}
			}
			break;
		case 0x90: // Note on
			rb_pop(&midinote);
			rb_pop(&midivel);
			if(midich != 9)
			{
				if(midivel != 0)
				{
					// check note is already on
					override = 0;
					for(int i = 0; i < CHANNEL_COUNT; ++ i)
					{
						if((beep[i].psg_midi_inuse == 1) && (beep[i].psg_midi_inuse_ch == midich) && (beep[i].psg_midi_note == midinote))
						{
							override=1;
						}
					}
					if(override == 0)
					{
						for(int i = 0; i < CHANNEL_COUNT; ++ i)
						{
							if(beep[i].psg_midi_inuse == 0)
							{
								noteon(i, midinote,midi_ch_volume[midich]);
								beep[i].psg_midi_inuse = 1;
								beep[i].psg_midi_inuse_ch = midich;
								beep[i].psg_midi_note = midinote;
								break;
							}
						}
					}
				} else {
					for(int i = 0; i < CHANNEL_COUNT; ++ i)
					{
						if((beep[i].psg_midi_inuse == 1) && (beep[i].psg_midi_inuse_ch == midich) && (beep[i].psg_midi_note == midinote))
						{
							noteoff(i, midinote);
							beep[i].psg_midi_inuse = 0;
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
						NoiseDrumSetPlay(&drum, rythmNote);
					}
				}
			}
			break;
		case 0xB0:
			// Channel control
			rb_pop(&midicc1);
			switch(midicc1)
			{
			case 7:
			case 11: // Expression
				rb_pop(&midicc2);
				midi_ch_volume[midich] = (midicc2 >> 3);
				if(midich == 9)
				{
					NoiseDrumSetVolume(&drum, midi_ch_volume[midich]);
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
					if((beep[i].psg_midi_inuse == 1) && (beep[i].psg_midi_inuse_ch == midich))
					{
						noteoff(i, beep[i].psg_midi_note);
						beep[i].psg_midi_inuse = 0;
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
				if((beep[i].psg_midi_inuse == 1) && (beep[i].psg_midi_inuse_ch == midich))
				{
					noteoff(i, beep[i].psg_midi_note);
					beep[i].psg_midi_inuse = 0;
				}
			}
			break;
		default: // Skip
			break;
		}
		tight_loop_contents();
	}
*/
}

// Core0の処理
int main()
{
	stdio_init_all();
	stdio_uart_init();

	// core1設定
//@@	multicore_launch_core1(core1_entry);

	// UART初期化
	uart_init(uart0, 9600);
	gpio_set_function(0, GPIO_FUNC_UART);  // TX = GP0
	gpio_set_function(1, GPIO_FUNC_UART);  // RX = GP1

	// USBスタック初期化
	tusb_init();
	uint8_t packet[4];
	int phase = 0;
	while(true)
	{
		tud_task();	// USBイベント処理
		if (tud_midi_available())
		{
			tud_midi_stream_read(packet, sizeof(packet));
			uint8_t cin	= packet[0] & 0x0F;
			uint8_t status = packet[1];
			uint8_t data1  = packet[2];
			uint8_t data2  = packet[3];
			uint8_t channel = status & 0x0F;
			uint8_t type	= status & 0xF0;
//			printf("Raw: %02X %02X %02X %02X | Ch:%02d ", packet[0], packet[1], packet[2], packet[3], channel);
			// リアルタイムメッセージ処理
			if (status >= 0xF8 && status <= 0xFF) {
				rb_push(status);
				continue;
			}
			switch(phase)
			{
			// チャンネルメッセージ処理
			case 0:
				if(status >= 0x80 && status <= 0xEF)
				{
					// チャンネルメッセージ
					rb_push(status);
					rb_push(data1);
					if(status < 0xC0 || status > 0xDF)
					{
						rb_push(data2);
					}
				}
				else if(status == 0xF0)
				{
					// SysExメッセージ開始
					rb_push(status);
					if(cin == 4)
					{
						rb_push(data1);
						rb_push(data2);
						if(data2 == 0xF7)
						{
							phase = 0;
						}
						else
						{
							phase = 1;
						}
					}
					else if(cin == 5)
					{
						rb_push(data1);
						if(data1 == 0xF7)
						{
							phase = 0;
						}
						else
						{
							phase = 1;
						}
					}
					else
					{
						phase = 1;
					}
				}
				break;
			// SysExメッセージ処理
			case 1:
				if(status == 0xF7)
				{
					rb_push(status);
					phase = 0;
				}
				else if(data1 == 0xF7)
				{
					rb_push(status);
					rb_push(data1);
					phase = 0;
				}
				else if(data2 == 0xF7)
				{
					rb_push(status);
					rb_push(data1);
					rb_push(data2);
					phase = 0;
				}
				else
				{
					rb_push(status);
					rb_push(data1);
					rb_push(data2);
				}
			}
		}
	}
}
