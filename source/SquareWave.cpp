#include <string.h>
#include "SquareWave.hpp"

// Volume table
const uint16_t SquareWave::psgVolume[] =
{
	0x00, 0x00, 0x17, 0x20, 0x27, 0x30, 0x37, 0x40,
	0x47, 0x50, 0x57, 0x60, 0x67, 0x70, 0x77, 0x80, 0x87, 0x90, 0x97, 0xa0,
	0xa7, 0xb0, 0xb7, 0xc0, 0xc7, 0xd0, 0xd7, 0xe0, 0xe7, 0xf0, 0xf7, 0xff
};

// Note毎の周波数の半分の値
const uint32_t SquareWave::toneIntervalHalf[] =
{
	122312, 115447, 108967, 102851, 97079, 91630, 86487, 81633,
	77051, 72727, 68645, 64792, 61156, 57723, 54483, 51425,
	48539, 45815, 43243, 40816, 38525, 36363, 34322, 32396,
	30578, 28861, 27241, 25712, 24269, 22907, 21621, 20408,
	19262, 18181, 17161, 16198, 15289, 14430, 13620, 12856,
	12134, 11453, 10810, 10204, 9631, 9090, 8580, 8099,
	7644, 7215, 6810, 6428, 6067, 5726, 5405, 5102,
	4815, 4545, 4290, 4049, 3822, 3607, 3405, 3214,
	3033, 2863, 2702, 2551, 2407, 2272, 2145, 2024,
	1911, 1803, 1702, 1607, 1516, 1431, 1351, 1275,
	1203, 1136, 1072, 1012, 955, 901, 851, 803,
	758, 715, 675, 637, 601, 568, 536, 506,
	477, 450, 425, 401, 379, 357, 337, 318,
	300, 284, 268, 253, 238, 225, 212, 200,
	189, 178, 168, 159, 150, 142, 134, 126,
	119, 112, 106, 100, 94, 89, 84, 79
};

SquareWave::SquareWave(void)
:oscIntervalHalf(UINT32_MAX)
,oscInterval(UINT32_MAX)
,oscCounter(0)
,toneOn(0)
,toneVolume(0)
,midiInUse(false)
,midiChannel(0)
,midiNote(0)
{
}

SquareWave::~SquareWave(void)
{
}

void SquareWave::Reset(void)
{
	this->oscIntervalHalf = UINT32_MAX;
	this->oscInterval = UINT32_MAX;
	this->oscCounter = 0;
	this->toneOn = 0;
	this->midiInUse = false;
}

void SquareWave::NoteOn(uint8_t note, uint8_t volume)
{
	this->oscIntervalHalf = toneIntervalHalf[note];
	this->oscInterval = toneIntervalHalf[note] << 1;
	this->oscCounter = 0;
	this->toneOn = 1;
	this->toneVolume = volume;
	this->midiNote = note;
	this->midiInUse = true;
}

void SquareWave::NoteOff(void)
{
	this->oscIntervalHalf = UINT32_MAX;
	this->oscInterval = UINT32_MAX;
	this->toneOn = 0;
	this->midiInUse = false;
}

uint8_t SquareWave::GetData(void)
{
	uint32_t pon_count = this->oscCounter += SAMPLING_INTERVAL;
	if(pon_count < (this->oscIntervalHalf))
	{
		return this->toneOn;
	}
	else if(pon_count > this->oscInterval)
	{
		this->oscCounter -= this->oscInterval;
		return this->toneOn;
	}
	else
	{
		return 0;
	}
}
