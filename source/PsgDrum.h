#pragma once
#include <stdint.h>
#include <stddef.h>

namespace psg_detail {
// Sequenced PSG effects, rendered at 32 kHz.
class SequencedVoice
{
public:
	struct Effect
	{
		uint32_t time;
		uint16_t toneFrequency;
		uint16_t noiseFrequency;
		uint8_t mixControl;
		uint8_t volume;
		uint32_t envelopeFrequency;
		uint8_t envelopePattern;
		uint16_t toneSweep;
		uint32_t noiseSweepCount;
		uint16_t noiseSweepData;
	};
	struct EffectData
	{
		size_t dataCount;
		const Effect* data;
	};
	static const int EFFECT_COUNT = 11;
	SequencedVoice(void);
	~SequencedVoice(void);
	void SetPlay(uint8_t index, uint8_t volume);
	void SetVolume(uint8_t volume);
	uint8_t GetData(void);
    void Stop() { phase = 2; effectData = nextEffectData = nullptr; }
    bool IsPlaying() const { return phase != 2 && (effectData || nextEffectData); }
private:
	static const int TIME_UNIT = 2000000;
	static const int OUTPUT_SAMPLING_FREQUENCY = 32000;
	static const int FPS60_INTERVAL = (TIME_UNIT / 60);
	static const int FPS60_SAMPLING = (OUTPUT_SAMPLING_FREQUENCY / 60);
	static const int FREQUENCY_SCALE = 16;
	static const int ENVELOPE_FREQUENCY_SCALE = 256;
	static const int INTERVAL = (TIME_UNIT / OUTPUT_SAMPLING_FREQUENCY);
	static const int INTERVAL60 = (TIME_UNIT / 60);
	static const uint8_t volumeTable[];
	static const Effect effect000[];
	static const Effect effect001[];
	static const Effect effect002[];
	static const Effect effect003[];
	static const Effect effect004[];
	static const Effect effect005[];
	static const Effect effect006[];
	static const Effect effect007[];
	static const Effect effect008[];
	static const Effect effect009[];
	static const Effect effect010[];
	static const EffectData effectDatas[];
	unsigned char Rnd(void);
	void InitializePhase(void);
	void NextData(void);
	uint32_t counter;
	uint8_t playIndex;
	const EffectData* effectData;
	uint8_t nextPlayIndex;
	const EffectData* nextEffectData;
	uint8_t phase;
	uint8_t volume;
	uint8_t masterVolume;
	// Noise
	uint32_t noiseInterval;
	uint32_t noiseReleaseCounter;
	uint8_t noiseBeforeData;
	uint32_t noiseSweepCounter;
	// Tone
	uint32_t toneIntervalHalf;
	uint32_t toneInterval;
	uint32_t toneCounter;
	uint32_t toneSweepCounter;
	// rnd
	static unsigned short rndSeed;
};

class ProceduralVoice {
public:
    struct Preset {
        uint8_t note;
        uint16_t tone1, tone2, noise, milliseconds;
        int16_t sweep_hz;
        uint16_t pulse_ms;
        uint8_t pulses, tone_mix, peak;
    };
    static const Preset presets[31];
    static const Preset* find(uint8_t note);
    bool playing() const;
    void stop();
    void start(const Preset& p);
    int16_t sample();
private:
    static uint32_t step(uint16_t hz);
    uint32_t remaining=0, phase1=0, phase2=0, noise_phase=0;
    uint32_t step1=0, step2=0, noise_step=0, rng=1;
    uint32_t envelope=0, drop=0, pulse_frames=0, pulse_left=0;
    int32_t sweep=0, noise_value=1;
    uint8_t tick=0, blend=0, peak=0, pulses_left=0;
};

}

// GM percussion notes 35..81, with original synthesis and voice allocation preserved.
class PsgDrum {
public:
    bool start(uint8_t channel, uint8_t note, uint8_t velocity,
               uint16_t gain, uint8_t volume, uint8_t expression);
    void stop(int channel = -1);
    unsigned active_count(int channel = -1) const;
    void update_gain(uint8_t channel, uint16_t gain, uint8_t volume, uint8_t expression);
    int32_t sample(uint16_t master);
private:
    struct SequencedSlot {
        psg_detail::SequencedVoice engine;
        uint8_t channel = 9;
        uint32_t clock = 0;
        uint8_t sample = 0;
    };
    struct ProceduralSlot {
        psg_detail::ProceduralVoice engine;
        uint32_t serial = 0;
        uint16_t gain = 0;
        uint8_t channel = 9, note = 0, velocity = 0;
    };
    SequencedSlot sequenced[psg_detail::SequencedVoice::EFFECT_COUNT];
    ProceduralSlot procedural[6];
    uint32_t serial = 0;
};
