#ifndef SINWAVE_HPP
#define SINWAVE_HPP

#include <stdint.h>

class SinWave
{
public:
	static const int TIME_UNIT = 2000000;
	static const int OUTPUT_SAMPLING_FREQUENCY = 32000;
	static const int SAMPLING_INTERVAL = (TIME_UNIT/OUTPUT_SAMPLING_FREQUENCY);
	static const int TABLE_SIZE = 256;
	static const int FP_SHIFT = 16;
	SinWave(void);
	~SinWave(void);
	void Reset(void);
	void NoteOn(uint8_t note, uint8_t volume);
	void NoteOff(void);
	uint8_t GetData(uint32_t volume);
	inline uint8_t GetNote(void)
	{
		return this->midiNote;
	}
	inline uint8_t GetToneOn(void)
	{
		return this->toneOn;
	}
	inline uint8_t GetToneVolume(void)
	{
		return this->toneVolume;
	}
	inline bool IsInUse(void)
	{
		return this->midiInUse;
	}
	inline void SetChannel(uint8_t channel)
	{
		this->midiChannel = channel;
	}
	inline uint8_t GetChannel(void)
	{
		return this->midiChannel;
	}
	static inline uint16_t getVolume(int volumeIndex)
	{
		return SinWave::psgVolume[volumeIndex];
	}
private:
	static const uint16_t psgVolume[];
	static const uint32_t midiPhaseIncTable[];
	static uint8_t sinTable[TABLE_SIZE];
	uint8_t toneOn;
	uint8_t toneVolume;
	bool midiInUse;
	uint8_t midiChannel;
	uint8_t midiNote;
	volatile uint32_t phase;
	volatile uint32_t phaseInc;
};

#endif
