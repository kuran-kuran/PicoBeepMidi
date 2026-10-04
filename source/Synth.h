#pragma once
#include "MidiParser.h"
#include "SoundBank.h"
#include "Cmu800Tone.h"
#include "PsgDrum.h"
#include <stddef.h>
#define PICOGM_VOICE_COUNT 32

class Synth : public MidiSink {
public:
    enum class Mode : uint8_t { gm, beep };
    void set_mode(Mode mode);
    Mode mode() const { return current_mode; }
    static constexpr unsigned sample_rate = 48000;
    static constexpr unsigned voice_count = PICOGM_VOICE_COUNT;
    static_assert(voice_count == 24 || voice_count == 32 || voice_count == 48,
                  "Supported voice counts: 24, 32, 48");
    struct Channel {
        uint8_t program = 0, volume = 100, expression = 127;
        uint8_t bank_msb = 0, bank_lsb = 0, pan = 64;
        uint8_t rpn_msb = 127, rpn_lsb = 127;
        uint8_t bend_semitones = 2, bend_cents = 0;
        int16_t bend = 0;
        bool pedal = false, mono = false, rhythm = false;
        uint16_t gain = 0;
        uint32_t bend_fraction_q24 = 1u << 24;
        int8_t bend_octave = 0;
    };
    Synth();
    void receive(const MidiMessage& message) override;
    void reset();
    void all_sound_off(int channel = -1);
    int16_t render();
    void render_block(int16_t* output, unsigned frames);
    unsigned active_count(int channel = -1) const;
    const Channel& channel(unsigned n) const { return channels[n]; }
    uint32_t phase_step_for_note(uint8_t ch, uint8_t note) const { return pitch_step(ch,note); }
    unsigned held_count() const;
    // Cumulative since boot, intentionally retained through MIDI resets.
    uint32_t note_ons = 0, note_offs = 0, unmatched_offs = 0, voice_steals = 0;
private:
    struct Voice {
        Cmu800Tone cmu;
        enum class Kind : uint8_t { gm, cmu, square, hit } kind = Kind::gm;
        const int8_t* wave = nullptr;
        uint32_t phase = 0, step = 0, position = 0, length = 0, stored_length = 0;
        uint32_t level = 0, elapsed = 0, duration = 0, factor = 0;
        uint32_t age = 0;
        uint16_t gain = 0;
        uint8_t channel = 0, note = 0, velocity = 0, program = 0, choke = 0;
        bool key_down = false, releasing = false, drum = false, infinite = false;
    };
    Channel channels[16];
    Voice voices[voice_count];
    Cmu800Tone cmu_templates[3];
    PsgDrum psg_drums;
    Mode current_mode = Mode::gm;
    uint16_t master = 16383;
    uint32_t age = 0, envelope_clock = 0;
    void note_on(uint8_t ch, uint8_t note, uint8_t velocity);
    void note_off(uint8_t ch, uint8_t note);
    void release(Voice& voice);
    void control(uint8_t ch, uint8_t cc, uint8_t value);
    void reset_controllers(uint8_t ch);
    void update_gain(uint8_t ch);
    void update_pitch(uint8_t ch);
    void voice_gain(Voice& voice);
    uint32_t pitch_step(uint8_t ch, uint8_t note) const;
    uint16_t cmu_divider(uint8_t ch, uint8_t note) const;
    uint32_t hit_step(uint8_t ch, uint8_t note, uint8_t program = 55) const;
    static int32_t render_hit(Voice& voice);
};
