#ifndef SQUAREWAVE_HPP
#define SQUAREWAVE_HPP

#include <stdint.h>

// Beep structure
class SquareWave
{
public:
	SquareWave(void);
	~SquareWave(void);
	void Reset(void);
	void NoteOn(uint8_t note, uint8_t volume);
	void NoteOff(uint8_t note);
	uint8_t GetData(void);
	static const uint16_t psgVolume[];
	uint8_t psg_midi_inuse;
	uint8_t psg_midi_inuse_ch;
	uint8_t psg_midi_note;
	uint8_t psg_tone_on;
private:
	static const int TIME_UNIT = 2000000;
	static const int OUTPUT_SAMPLING_FREQUENCY = 32000;
	static const int SAMPLING_INTERVAL = (TIME_UNIT/OUTPUT_SAMPLING_FREQUENCY);
	static const uint32_t toneIntervalHalf[];
	uint32_t psg_osc_intervalHalf;
	uint32_t psg_osc_interval;
	uint32_t psg_osc_counter;
};

#endif
