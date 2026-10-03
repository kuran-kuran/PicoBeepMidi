#pragma once
#include <stdint.h>

struct TonePreset {
    int32_t decay;
    int32_t release;
    uint32_t decay_factor_q31;
    uint32_t release_factor_q31;
    uint16_t gain_q15;
};
struct DrumPreset {
    const int8_t* wave;
    uint32_t length;
    uint8_t choke;
    uint32_t stored_length;
};
extern const int8_t* const tone_waves[128];
extern const TonePreset tone_presets[128];
extern const DrumPreset drum_presets[47];
extern const uint32_t midi_note_steps[128];
extern const uint32_t bend_ratios_q24[257];
