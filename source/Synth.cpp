#include "Synth.h"
#include "Cmu800Midi.h"
#include "SynthSamples.h"
#include <cstring>

Synth::Synth()
{
    for(unsigned i = 0; i < 3; ++i) configure_cmu(cmu_templates[i],i);
    reset();
}

void Synth::set_mode(Mode mode)
{
    if(mode == current_mode) return;
    all_sound_off();
    current_mode = mode;
}

void Synth::reset()
{
    all_sound_off(); master = 16383; age = 0; envelope_clock = 0;
    for(uint8_t ch = 0; ch < 16; ++ch) { channels[ch] = Channel{}; update_gain(ch); }
    channels[9].rhythm = true;
}

void Synth::all_sound_off(int ch)
{
    for(auto& v : voices) if(ch < 0 || v.channel == ch) v = Voice{};
    psg_drums.stop(ch);
}

unsigned Synth::active_count(int ch) const
{
    unsigned count = 0;
    for(const auto& v : voices) if(v.wave && (ch < 0 || v.channel == ch)) ++count;
    count += psg_drums.active_count(ch);
    return count;
}

void Synth::voice_gain(Voice& v)
{
    uint32_t gain = static_cast<uint32_t>(channels[v.channel].gain) * v.velocity / 127;
    if(!v.drum && v.kind == Voice::Kind::gm) gain = gain * tone_presets[v.program].gain_q15 >> 15;
    v.gain = static_cast<uint16_t>(gain);
}

void Synth::update_gain(uint8_t ch)
{
    auto& c = channels[ch];
    c.gain = static_cast<uint16_t>(static_cast<uint64_t>(c.volume)*c.expression*master*32767 / (127ull*127*16383));
    for(auto& v : voices) if(v.wave && v.channel == ch) voice_gain(v);
    psg_drums.update_gain(ch,c.gain,c.volume,c.expression);
}

uint16_t Synth::cmu_divider(uint8_t ch, uint8_t note) const
{
    // The original CMU oscillator accepts a 16-bit 8253 divider.
    uint64_t divider = note >= 24 ? cmu_counter_table[note-24] :
        ((1269866ull<<32)/sample_rate + midi_note_steps[note]/2)/midi_note_steps[note];
    const auto& c = channels[ch];
    divider = (divider<<24)/c.bend_fraction_q24;
    if(c.bend_octave >= 0) divider >>= c.bend_octave;
    else divider <<= -c.bend_octave;
    return static_cast<uint16_t>(divider > 65535 ? 65535 : (divider < 1 ? 1 : divider));
}

uint32_t Synth::pitch_step(uint8_t ch, uint8_t note) const
{
    const auto& c = channels[ch];
    uint64_t step = (static_cast<uint64_t>(midi_note_steps[note])*c.bend_fraction_q24) >> 24;
    if(c.bend_octave >= 0) step <<= c.bend_octave;
    else step >>= -c.bend_octave;
    return step < 0x80000000ull ? static_cast<uint32_t>(step) : 0;
}

void Synth::update_pitch(uint8_t ch)
{
    auto& c = channels[ch];
    // Exact range scaling followed by a small interpolated octave table.
    // No pow()/software double arithmetic in MIDI event processing on Cortex-M0+.
    const int32_t octave_q16 = static_cast<int32_t>(c.bend)*(c.bend_semitones*100+c.bend_cents)*8/1200;
    int32_t octave = octave_q16/65536, fraction = octave_q16%65536;
    if(fraction < 0) { fraction += 65536; --octave; }
    const uint32_t index = static_cast<uint32_t>(fraction)>>8;
    c.bend_octave = static_cast<int8_t>(octave);
    c.bend_fraction_q24 = bend_ratios_q24[index] +
        ((bend_ratios_q24[index+1]-bend_ratios_q24[index])*(static_cast<uint32_t>(fraction)&255) >> 8);
    for(auto& v : voices) if(v.wave && v.channel == ch && !v.drum) {
        v.step = v.kind == Voice::Kind::hit ? hit_step(ch,v.note,v.program) : pitch_step(ch,v.note);
        if(v.kind == Voice::Kind::cmu) v.cmu.set_8253(cmu_divider(ch,v.note));
    }
}

uint32_t Synth::hit_step(uint8_t ch, uint8_t note, uint8_t program) const
{
    // 24 kHz sample at C4, rendered at 48 kHz; Q16 sample increment.
    const uint32_t root = program == 14 ? tubular_bell_root_step : midi_note_steps[60];
    return static_cast<uint32_t>(static_cast<uint64_t>(pitch_step(ch,note))*32768/root);
}

int32_t Synth::render_hit(Voice& v)
{
    const uint32_t next = v.position+1 < v.length ? v.position+1 : (v.infinite ? 0 : v.position);
    const int32_t a = v.position < v.stored_length ? v.wave[v.position] : 0;
    const int32_t b = next < v.stored_length ? v.wave[next] : 0;
    const int32_t sample = a + (b-a)*static_cast<int32_t>(v.phase)/65536;
    int32_t gain = v.gain;
    if(v.releasing) gain = static_cast<int32_t>(static_cast<uint32_t>(gain)*(v.duration-v.elapsed)/v.duration);
    const int32_t output = v.step ? sample*gain/128 : 0;
    const uint32_t advance = v.phase+v.step;
    v.position += advance>>16; v.phase = advance&65535;
    if(v.infinite && v.position >= v.length) v.position %= v.length;
    if(v.position >= v.length || (v.releasing && ++v.elapsed >= v.duration) || !v.step) v.wave = nullptr;
    return output;
}

unsigned Synth::held_count() const
{
    unsigned count = 0;
    for(const auto& v : voices) if(v.wave && v.key_down && !v.drum) ++count;
    return count;
}

void Synth::release(Voice& v)
{
    if(v.drum || v.releasing || !v.wave) return;
    if(v.kind == Voice::Kind::square) { v.wave = nullptr; return; }
    if(v.kind == Voice::Kind::hit) { v.releasing = true; v.elapsed = 0; v.duration = sample_rate*120/1000; return; }
    if(v.kind == Voice::Kind::cmu) { v.releasing = true; v.cmu.set_gate(false); return; }
    v.releasing = true; v.infinite = false; v.elapsed = 0;
    v.duration = static_cast<uint32_t>(tone_presets[v.program].release);
    v.factor = tone_presets[v.program].release_factor_q31;
    if(!v.duration) v.wave = nullptr;
}

void Synth::note_on(uint8_t ch, uint8_t note, uint8_t velocity)
{
    if(!velocity) { note_off(ch,note); return; }
    if(channels[ch].rhythm && (note < 35 || note > 81)) return;
    if(current_mode == Mode::beep && channels[ch].rhythm) {
        const auto& c = channels[ch];
        if(psg_drums.start(ch,note,velocity,c.gain,c.volume,c.expression)) ++voice_steals;
        ++note_ons;
        return;
    }
    ++note_ons;
    if(channels[ch].mono && !channels[ch].rhythm) all_sound_off(ch);
    // The emulator expresses oscillator pitch changes as NoteOff/NoteOn.
    // Replace CMU release tails instead of stacking a new oscillator on each change.
    // Keep held keys (including pedal-held notes) available for MIDI chords.
    if(current_mode == Mode::gm && !channels[ch].rhythm && channels[ch].program == 7)
        for(auto& old : voices)
            if(old.wave && old.channel == ch && old.kind == Voice::Kind::cmu && old.releasing)
                old = Voice{};
    const DrumPreset* drum = channels[ch].rhythm ? &drum_presets[note-35] : nullptr;
    if(drum && drum->choke)
        for(auto& v : voices) if(v.wave && v.drum && v.channel == ch && v.choke == drum->choke) v.wave = nullptr;
    Voice* selected = &voices[0];
    for(auto& v : voices) {
        if(!v.wave) { selected = &v; break; }
        if(v.age < selected->age) selected = &v;
    }
    auto& v = *selected;
    if(v.wave) ++voice_steals;
    v = Voice{};
    v.channel = ch; v.note = note; v.velocity = velocity; v.key_down = true;
    v.program = channels[ch].program; v.age = ++age; v.level = 0x7fffffff;
    if(drum) {
        v.drum = true; v.wave = drum->wave; v.length = drum->length; v.choke = drum->choke;
        v.stored_length = drum->stored_length;
        if(note == 43 || note == 45 || note == 47) {
            v.wave = noisy_low_tom;
            v.stored_length = sizeof(noisy_low_tom);
        }
        // Tune the measured CMU sample resonances (low 142.456 Hz, high
        // 189.697 Hz) to a wider GM tom range. Q16 source-sample increments.
        // Keep drum NoteOff/bend semantics while using interpolated playback.
        switch(note) {
        case 41: v.step = 33775; break; // 73.416 Hz
        case 43: v.step = 75822; break; // 164.814 Hz
        case 45: v.step = 90168; break; // 195.998 Hz
        case 47: v.step = 120360; break; // 261.626 Hz
        case 48: v.step = 50727; break; // 146.832 Hz
        case 50: v.step = 56939; break; // 164.814 Hz
        default: break;
        }
        if(note == 41 || note == 43 || note == 45 || note == 47 || note == 48 || note == 50)
            v.kind = Voice::Kind::hit;
    } else if(current_mode == Mode::beep) {
        v.kind = Voice::Kind::square;
        v.wave = tone_waves[0]; // Non-null active marker; square samples are computed.
        v.step = pitch_step(ch,note);
        v.infinite = true;
    } else if(v.program >= 116) {
        const auto& sample = special_samples[v.program-116];
        v.kind = Voice::Kind::hit;
        v.wave = sample.wave; v.length = sample.length; v.infinite = sample.loop;
        v.stored_length = sample.stored_length;
        v.step = hit_step(ch,note);
    } else if(v.program == 14) {
        v.kind = Voice::Kind::hit;
        v.wave = tubular_bell; v.length = sizeof(tubular_bell);
        v.stored_length = v.length;
        v.step = hit_step(ch,note,v.program);
    } else if(v.program == 55) {
        v.kind = Voice::Kind::hit;
        v.wave = orchestra_hit; v.length = sizeof(orchestra_hit);
        v.stored_length = v.length;
        v.step = hit_step(ch,note);
    } else if(v.program == 7) {
        v.kind = Voice::Kind::cmu;
        v.wave = ch == 1 ? Cmu800Tone::bass_table : Cmu800Tone::melody_table;
        v.cmu = cmu_templates[ch < 2 ? ch : 2];
        v.cmu.set_8253(cmu_divider(ch,note));
        v.cmu.set_gate(true);
        v.step = pitch_step(ch,note);
        // The unchanged envelope can plateau below one output LSB due to Q31
        // rounding. Retire that inaudible voice after 16 s at the fixed decay.
        v.duration = sample_rate*16;
    } else {
        const auto& p = tone_presets[v.program];
        v.wave = tone_waves[v.program]; v.step = pitch_step(ch,note);
        v.infinite = p.decay < 0; v.duration = static_cast<uint32_t>(p.decay < 0 ? 0 : p.decay);
        v.factor = p.decay_factor_q31;
        if(!v.infinite && !v.duration) v.wave = nullptr;
    }
    voice_gain(v);
}

void Synth::note_off(uint8_t ch, uint8_t note)
{
    ++note_offs;
    // Repeated NoteOns get independent voices; match the oldest still-held key.
    Voice* match = nullptr;
    for(auto& v : voices) if(v.wave && v.channel == ch && v.note == note && v.key_down)
        if(!match || v.age < match->age) match = &v;
    if(match) {
        match->key_down = false;
        if(!channels[ch].pedal) release(*match);
    } else ++unmatched_offs;
}

void Synth::reset_controllers(uint8_t ch)
{
    auto& c = channels[ch];
    c.expression = 127; c.pedal = false; c.bend = 0; c.rpn_msb = c.rpn_lsb = 127;
    // Preserve volume, pan, bank, program and pitch-bend sensitivity.
    for(auto& v : voices) if(v.wave && v.channel == ch && !v.key_down) release(v);
    update_gain(ch); update_pitch(ch);
}

void Synth::control(uint8_t ch, uint8_t cc, uint8_t value)
{
    auto& c = channels[ch];
    switch(cc) {
    case 0: c.bank_msb = value; break;
    case 32: c.bank_lsb = value; break;
    case 7: c.volume = value; update_gain(ch); break;
    case 10: c.pan = value; break; // Mono output: retain state without spatial effect.
    case 11: c.expression = value; update_gain(ch); break;
    case 64:
        c.pedal = value >= 64;
        if(!c.pedal) for(auto& v : voices) if(v.wave && v.channel == ch && !v.key_down) release(v);
        break;
    case 100: c.rpn_lsb = value; break;
    case 101: c.rpn_msb = value; break;
    case 98: case 99: c.rpn_msb = c.rpn_lsb = 127; break; // Unsupported NRPN cancels RPN selection.
    case 6: case 38:
        if(c.rpn_msb == 0 && c.rpn_lsb == 0) {
            if(cc == 6) c.bend_semitones = value;
            else c.bend_cents = value > 99 ? 99 : value;
            update_pitch(ch);
        }
        break;
    case 120: all_sound_off(ch); break;
    case 121: reset_controllers(ch); break;
    case 123: case 124: case 125: case 126: case 127:
        for(auto& v : voices) if(v.wave && v.channel == ch) {
            v.key_down = false;
            if(!c.pedal) release(v);
        }
        if(cc == 126) c.mono = true;
        if(cc == 127) c.mono = false;
        break;
    default: break; // Unsupported messages are fully parsed and safely ignored.
    }
}

void Synth::receive(const MidiMessage& m)
{
    if(m.status == 0xff) { reset(); return; }
    if(m.status == 0xf0) {
        if(m.size == 4 && m.data[0] == 0x7e && m.data[2] == 9 && m.data[3] == 1) reset();
        if(m.size == 6 && m.data[0] == 0x7f && m.data[2] == 4 && m.data[3] == 1) {
            master = static_cast<uint16_t>(m.data[4] | (m.data[5] << 7));
            for(uint8_t ch = 0; ch < 16; ++ch) update_gain(ch);
        }
        const uint8_t gs[] = {0x41,0x10,0x42,0x12,0x40,0x00,0x7f,0x00,0x41};
        if(m.size == sizeof(gs) && std::memcmp(m.data,gs,sizeof(gs)) == 0) reset();
        // XG System On uses the same GM defaults; accept device numbers 0..15.
        if(m.size == 7 && m.data[0] == 0x43 && (m.data[1]&0xf0) == 0x10 &&
           m.data[2] == 0x4c && m.data[3] == 0 && m.data[4] == 0 &&
           m.data[5] == 0x7e && m.data[6] == 0) reset();
        // GS USE FOR RHYTHM PART (401x15): part 10 is block 0,
        // parts 1..9 are blocks 1..9; parts 11..16 are blocks A..F.
        // Both rhythm maps use our single GM kit; kit variations are not emulated.
        if(m.size == 9 && m.data[0] == 0x41 && m.data[1] == 0x10 &&
           m.data[2] == 0x42 && m.data[3] == 0x12 && m.data[4] == 0x40 &&
           (m.data[5]&0x70) == 0x10 && m.data[6] == 0x15 && m.data[7] <= 2) {
            unsigned checksum = 0;
            for(unsigned i = 4; i < 9; ++i) checksum += m.data[i];
            if((checksum&127) == 0) {
                const uint8_t part = m.data[5]&15;
                const uint8_t ch = part == 0 ? 9 : (part < 10 ? part-1 : part);
                all_sound_off(ch);
                channels[ch].rhythm = m.data[7] != 0;
            }
        }
        return;
    }
    const uint8_t ch = m.status & 15, kind = m.status & 0xf0;
    if(m.status < 0x80 || m.status >= 0xf0) return;
    if(kind == 0xc0 && m.size == 1) { channels[ch].program = m.data[0]; return; }
    if(m.size != 2) return;
    switch(kind) {
    case 0x80: note_off(ch,m.data[0]); break;
    case 0x90: note_on(ch,m.data[0],m.data[1]); break;
    case 0xb0: control(ch,m.data[0],m.data[1]); break;
    case 0xe0: channels[ch].bend = static_cast<int16_t>((m.data[0] | (m.data[1]<<7))-8192); update_pitch(ch); break;
    default: break;
    }
}

#ifdef PICO_ON_DEVICE
__attribute__((section(".time_critical.synth_render_block")))
#endif
void Synth::render_block(int16_t* output, unsigned frames)
{
    // Preserve PSG shared RNG/sample ordering in the already-fast BEEP path.
    if(current_mode == Mode::beep) {
        for(unsigned i = 0; i < frames; ++i) output[i] = render();
        return;
    }
    while(frames) {
        const unsigned count = frames < 128 ? frames : 128;
        int32_t mix[128] = {};
        const uint32_t clock = envelope_clock;
        envelope_clock += count;
        // Voice-major traversal keeps one waveform in the small XIP cache.
        // Voice type, gain, phase and length are not reloaded for every sample.
        for(auto& v : voices) {
            if(!v.wave) continue;
            if(v.kind == Voice::Kind::cmu) {
                const int32_t gain = v.step ? v.gain : 0;
                for(unsigned i = 0; i < count; ++i) {
                    mix[i] += v.cmu.get_data_with_volume(gain)*256;
                    if(!v.cmu.is_playing() || ++v.elapsed >= v.duration) {
                        v.cmu.stop(); v.wave = nullptr;
                        break;
                    }
                }
            } else if(v.kind == Voice::Kind::hit) {
                // Cache voice state for the whole block. Keep the sample in Flash.
                const int8_t* const wave = v.wave;
                const uint32_t length = v.length, step = v.step;
                uint32_t position = v.position, phase = v.phase, elapsed = v.elapsed;
                int32_t gain = v.gain;
                uint32_t remainder = 0, decrement = 0, remainder_step = 0;
                if(v.releasing) {
                    // Exact quotient/remainder recurrence: same rounding as the
                    // sample renderer, with two divisions per block, not per sample.
                    const uint32_t numerator = v.gain*(v.duration-elapsed);
                    gain = static_cast<int32_t>(numerator/v.duration);
                    remainder = numerator-static_cast<uint32_t>(gain)*v.duration;
                    decrement = v.gain/v.duration;
                    remainder_step = v.gain-decrement*v.duration;
                }
                for(unsigned i = 0; i < count; ++i) {
                    const uint32_t next = position+1 < length ? position+1 : (v.infinite ? 0 : position);
                    const int32_t a = position < v.stored_length ? wave[position] : 0;
                    const int32_t b = next < v.stored_length ? wave[next] : 0;
                    const int32_t sample = a + (b-a)*static_cast<int32_t>(phase)/65536;
                    if(step) mix[i] += sample*gain/128;
                    const uint32_t advance = phase+step;
                    position += advance>>16; phase = advance&65535;
                    // Modulo only at loop boundaries, never for ordinary samples.
                    if(v.infinite && position >= length) position %= length;
                    if(position >= length || (v.releasing && ++elapsed >= v.duration) || !step) {
                        v.wave = nullptr; break;
                    }
                    if(v.releasing) {
                        gain -= static_cast<int32_t>(decrement);
                        if(remainder < remainder_step) { --gain; remainder += v.duration; }
                        remainder -= remainder_step;
                    }
                }
                v.position = position; v.phase = phase; v.elapsed = elapsed;
            } else if(v.drum) {
                const uint32_t remaining = v.length-v.position;
                const unsigned n = remaining < count ? remaining : count;
                const int32_t gain = v.gain;
                const unsigned available = v.position < v.stored_length ? v.stored_length-v.position : 0;
                const unsigned audible = available < n ? available : n;
                for(unsigned i = 0; i < audible; ++i) mix[i] += static_cast<int32_t>(v.wave[v.position+i])*gain/128;
                v.position += n;
                if(v.position == v.length) v.wave = nullptr;
            } else {
                const int8_t* const wave = v.wave;
                const uint32_t step = v.step;
                uint32_t phase = v.phase;
                unsigned offset = 0;
                while(offset < count) {
                    // Split at exactly the same envelope ticks as render().
                    unsigned n = v.infinite ? count-offset : 32-((clock+offset)&31);
                    if(n > count-offset) n = count-offset;
                    if(!v.infinite && n > v.duration-v.elapsed) n = v.duration-v.elapsed;
                    const int32_t gain = static_cast<int32_t>((v.level>>16)*v.gain>>15);
                    if(step) for(unsigned i = 0; i < n; ++i) {
                        mix[offset+i] += static_cast<int32_t>(wave[phase>>20])*gain/128;
                        phase += step;
                    }
                    offset += n;
                    if(!v.infinite) {
                        v.elapsed += n;
                        if(v.elapsed >= v.duration) { v.wave = nullptr; break; }
                        if(((clock+offset)&31) == 0)
                            v.level = static_cast<uint32_t>((static_cast<uint64_t>(v.level)*v.factor)>>31);
                    }
                }
                v.phase = phase;
            }
        }
        for(unsigned i = 0; i < count; ++i) {
            const int32_t sample = mix[i]/4;
            output[i] = static_cast<int16_t>(sample < -32768 ? -32768 : (sample > 32767 ? 32767 : sample));
        }
        output += count;
        frames -= count;
    }
}

#ifdef PICO_ON_DEVICE
__attribute__((section(".time_critical.synth_render")))
#endif
int16_t Synth::render()
{
    // Envelope multiplication only every 32 samples; no division/floating point per voice per sample.
    const bool envelope_tick = (++envelope_clock & 31) == 0;
    int32_t mix = 0;
    for(auto& v : voices) {
        if(!v.wave) continue;
        if(v.kind == Voice::Kind::cmu) {
            mix += v.cmu.get_data_with_volume(v.step ? v.gain : 0)*256;
            if(!v.cmu.is_playing() || ++v.elapsed >= v.duration) { v.cmu.stop(); v.wave = nullptr; }
        } else if(v.kind == Voice::Kind::hit) {
            mix += render_hit(v);
        } else if(v.kind == Voice::Kind::square) {
            if(v.step) mix += (v.phase & 0x80000000u ? -127 : 127)*v.gain/128;
            v.phase += v.step;
        } else if(v.drum) {
            if(v.position < v.stored_length) mix += static_cast<int32_t>(v.wave[v.position]) * v.gain / 128;
            ++v.position;
            if(v.position >= v.length) v.wave = nullptr;
        } else {
            const int32_t gain = static_cast<int32_t>((v.level >> 16)*v.gain >> 15);
            if(v.step) mix += static_cast<int32_t>(v.wave[v.phase >> 20])*gain / 128;
            v.phase += v.step;
            if(!v.infinite) {
                if(++v.elapsed >= v.duration) v.wave = nullptr;
                else if(envelope_tick) v.level = static_cast<uint32_t>((static_cast<uint64_t>(v.level)*v.factor) >> 31);
            }
        }
    }
    if(current_mode == Mode::beep) mix += psg_drums.sample(master);
    mix /= 4; // Fixed headroom; no level pumping when voices enter or leave.
    // BEEP master gain: about -9 dB, applied before clipping to preserve headroom.
    if(current_mode == Mode::beep) mix = mix*91/256;
    return static_cast<int16_t>(mix < -32768 ? -32768 : (mix > 32767 ? 32767 : mix));
}
