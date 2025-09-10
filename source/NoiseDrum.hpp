#ifndef NOISEDRUM_H
#define NOISEDRUM_H

#include <stdint.h>

// NoiseDrum
class NoiseDrum
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
	NoiseDrum(void);
	~NoiseDrum(void);
	void SetPlay(uint8_t index);
	void SetVolume(uint8_t volume);
	uint8_t GetData(void);
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
	uint8_t phase;
	const EffectData* effectData;
	uint8_t volume;
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

#endif
