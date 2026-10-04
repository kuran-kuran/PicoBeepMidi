#pragma once
#include "pico.h"

#define AUDIO_PWM_PIN 6

// Board identity comes from the Pico SDK's PICO_BOARD selection.
// Set MODE_LED_PIN to -1 in the selected branch to disable the LED.
#if defined(WAVESHARE_RP2040_ZERO)
#define MODE_LED_PIN 16
#define MODE_LED_RGB 1
#elif defined(RASPBERRYPI_PICO)
#define MODE_LED_PIN 25
#define MODE_LED_RGB 0
#else
#error "Supported boards: pico, waveshare_rp2040_zero"
#endif
