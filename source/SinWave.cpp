#include <math.h>
#include <string.h>
#include "SinWave.hpp"

// Volume table
const uint16_t SinWave::psgVolume[] =
{
	0x00, 0x00, 0x17, 0x20, 0x27, 0x30, 0x37, 0x40,
	0x47, 0x50, 0x57, 0x60, 0x67, 0x70, 0x77, 0x80, 0x87, 0x90, 0x97, 0xa0,
	0xa7, 0xb0, 0xb7, 0xc0, 0xc7, 0xd0, 0xd7, 0xe0, 0xe7, 0xf0, 0xf7, 0xff
};

const uint32_t SinWave::midiPhaseIncTable[128] =
{
	4194, 4718, 4718, 5242, 5767, 6291, 6815, 7340,
	7864, 8388, 8912, 9437, 9961, 11010, 11534, 12058,
	13107, 14155, 15204, 15728, 16777, 17825, 18874, 19922,
	20971, 22544, 23592, 25165, 26738, 28311, 29884, 31457,
	33554, 35651, 37748, 39845, 42467, 45088, 47710, 50331,
	53477, 56623, 59768, 63438, 67108, 71303, 75497, 79691,
	84410, 89653, 94896, 100663, 106430, 112721, 119537, 126877,
	134217, 142082, 150470, 159383, 168820, 178782, 189792, 201326,
	212860, 225968, 239075, 253231, 267911, 283639, 300417, 317718,
	336068, 355467, 375914, 397410, 419954, 444071, 468713, 494927,
	522190, 551026, 580911, 611844, 644349, 678428, 713555, 750256,
	788529, 828375, 869793, 912785, 957349, 1003487, 1051197, 1101004,
	1151860, 1204813, 1259339, 1315962, 1374158, 1434451, 1496317, 1560281,
	1626341, 1693974, 1763704, 1835532, 1909456, 1985478, 2063597, 2144337,
	2226651, 2311585, 2398617, 2487746, 2579496, 2673344, 2769813, 2868379,
	2969567, 3072851, 3178758, 3287285, 3398434, 3511681, 3627548, 3746037
};

uint8_t SinWave::sinTable[TABLE_SIZE];

SinWave::SinWave(void)
:toneOn(0)
,toneVolume(0)
,midiInUse(false)
,midiChannel(0)
,midiNote(0)
,phase(0)
,phaseInc(0)
{
	for(int i = 0; i < TABLE_SIZE; ++ i)
	{
		double angle = (2.0 * M_PI * i) / TABLE_SIZE;
		SinWave::sinTable[i] = static_cast<uint8_t>((sin(angle) + 1.0) * 127.5);
	}
}

SinWave::~SinWave(void)
{
}

void SinWave::Reset(void)
{
	this->toneOn = 0;
	this->midiInUse = false;
}

void SinWave::NoteOn(uint8_t note, uint8_t volume)
{
	this->toneOn = 1;
	this->toneVolume = volume;
	this->midiNote = note;
	this->midiInUse = true;
	this->phaseInc = midiPhaseIncTable[note]; // (周波数 * TABLE_SIZE << FP_SHIFT) / OUTPUT_SAMPLING_FREQUENCY;
}

void SinWave::NoteOff(void)
{
	this->toneOn = 0;
	this->midiInUse = false;
}

uint8_t SinWave::GetData(uint32_t volume)
{
	if(this->toneOn == 0)
	{
		return 0;
	}
	this->phase += this->phaseInc;
	uint8_t index = (this->phase >> FP_SHIFT) & (TABLE_SIZE - 1);
	return static_cast<uint8_t>((sinTable[index] * this->toneVolume * volume) >> 16);
}
