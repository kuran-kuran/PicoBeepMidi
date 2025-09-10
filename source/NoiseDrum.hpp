#ifndef NOISEDRUM_H
#define NOISEDRUM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TIME_UNIT 2000000
#define OUTPUT_SAMPLING_FREQUENCY 32000
#define FPS60_INTERVAL (TIME_UNIT / 60)
#define FPS60_SAMPLING (OUTPUT_SAMPLING_FREQUENCY / 60)
#define FREQUENCY_SCALE 16
#define ENVELOPE_FREQUENCY_SCALE 256
#define INTERVAL (TIME_UNIT / OUTPUT_SAMPLING_FREQUENCY)
#define INTERVAL60 (TIME_UNIT / 60)

typedef struct Effect_
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
} Effect;

typedef struct EffectData_
{
    size_t dataCount;
    const Effect* data;
} EffectData;

// NoiseDrum
class NoiseDrum
{
public:
	NoiseDrum(void);
	~NoiseDrum(void);
	void SetPlay(uint8_t index);
	void SetVolume(uint8_t volume);
	uint8_t GetData(void);
private:
	unsigned char Rnd(void);
	void InitializePhase(void)
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

extern const EffectData psgEffectDatas[];

#ifdef __cplusplus
}
#endif

#endif
