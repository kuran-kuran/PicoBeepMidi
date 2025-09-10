#ifndef BEEP_H
#define BEEP_H

#include <stdint.h>

#define TIME_UNIT                 2000000
#define OUTPUT_SAMPLING_FREQUENCY 32000
#define PSG_DEVIDE_FACTOR         9
#define CHANNEL_COUNT             20
#define SAMPLING_INTERVAL         (TIME_UNIT/OUTPUT_SAMPLING_FREQUENCY)

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
private:
    uint32_t psg_osc_intervalHalf;
    uint32_t psg_osc_interval;
    uint32_t psg_osc_counter;
    uint8_t psg_tone_on;
    uint8_t psg_midi_inuse;
    uint8_t psg_midi_inuse_ch;
    uint8_t psg_midi_note;
};

#endif
