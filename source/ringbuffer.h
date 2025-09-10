#ifndef RINGBUFFER_H
#define RINGBUFFER_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#define BUF_SIZE 256			// 2のべき乗
#define BUF_MASK (BUF_SIZE - 1)	// &マスクに使う

static volatile uint8_t ringbuf[BUF_SIZE];
static volatile uint8_t head = 0;  // Core 0 only
static volatile uint8_t tail = 0;  // Core 1 only

// Core 0：データをリングバッファへ追加
static inline void rb_clear(void)
{
	head = 0;
	tail = 0;
}

// Core 0：データをリングバッファへ追加
static inline void rb_push(uint8_t val)
{
	uint8_t next = (head + 1) & BUF_MASK;
	// 満杯でなければ格納
	if(next != tail)
	{
		ringbuf[head] = val;
		head = next;
	}
}

// Core 1：データをリングバッファから取り出す
static inline bool rb_pop(uint8_t *out)
{
	// 空なら失敗
	if(head == tail)
	{
		return false;
	}
	*out = ringbuf[tail];
	tail = (tail + 1) & BUF_MASK;
	return true;
}

#endif
