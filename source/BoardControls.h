#pragma once
#include <stdint.h>

// Core 1 owns LED and button polling. Core 0 calls service from its main loop.
void board_controls_init();
void board_controls_service();
// False means no safe reading was obtained; it is not a button release.
bool board_bootsel_read(bool& down);
extern uint32_t board_button_timeouts;
extern uint32_t board_max_button_us;
void board_mode_led(bool beep);
// Core 1 only; callback runs with core 0 parked in SRAM.
bool board_flash_operation(void (*operation)(void*), void* context);
