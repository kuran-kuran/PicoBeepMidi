#include <string.h>
#include "NoiseDrum.hpp"

const uint8_t NoiseDrum::volumeTable[] =
{
	0, 10, 12, 16, 20, 25, 32, 40, 51, 64, 80, 101, 128, 161, 203, 255, 255
};

// Bass Drum
const NoiseDrum::Effect NoiseDrum::effect000[] =
{
	1 * INTERVAL60, 1500 * FREQUENCY_SCALE, 31 * FREQUENCY_SCALE, 54, 15, 0, 0, 127 * FREQUENCY_SCALE, 0, 0,
	8 * INTERVAL60, 1700 * FREQUENCY_SCALE, 1, 62, 16, 1200 * INTERVAL, 0, 127 * FREQUENCY_SCALE, 0, 0,
};

// Snare Drum
const NoiseDrum::Effect NoiseDrum::effect001[] =
{
	14 * INTERVAL60, 400 * FREQUENCY_SCALE, 7 * FREQUENCY_SCALE, 54, 16, 3000 * INTERVAL, 0, 93 * FREQUENCY_SCALE, 15 * INTERVAL60, 2 * FREQUENCY_SCALE,
};

// Low Tom
const NoiseDrum::Effect NoiseDrum::effect002[] =
{
	2 * INTERVAL60, 700 * FREQUENCY_SCALE, 1, 54, 15, 0, 0, 100 * FREQUENCY_SCALE, 0, 0,
	14 * INTERVAL60, 900 * FREQUENCY_SCALE, 1, 54, 16, 2500 * INTERVAL, 0, 100 * FREQUENCY_SCALE, 0, 0,
};

// Middle Tom
const NoiseDrum::Effect NoiseDrum::effect003[] =
{
	2 * INTERVAL60, 500 * FREQUENCY_SCALE, 5 * FREQUENCY_SCALE, 54, 15, 0, 0, 60 * FREQUENCY_SCALE, 0, 0,
	14 * INTERVAL60, 620 * FREQUENCY_SCALE, 1, 54, 16, 2500 * INTERVAL, 0, 60 * FREQUENCY_SCALE, 0, 0,
};

// High Tom
const NoiseDrum::Effect NoiseDrum::effect004[] =
{
	2 * INTERVAL60, 300 * FREQUENCY_SCALE, 1, 54, 15, 0, 0, 50 * FREQUENCY_SCALE, 0, 0,
	14 * INTERVAL60, 400 * FREQUENCY_SCALE, 1, 54, 16, 2500 * INTERVAL, 0, 50 * FREQUENCY_SCALE, 0, 0,
};

// Rim Shot
const NoiseDrum::Effect NoiseDrum::effect005[] =
{
	2 * INTERVAL60, 55 * FREQUENCY_SCALE, 1, 62, 16, 300 * INTERVAL, 0, 100 * FREQUENCY_SCALE, 0, 0,
};

// Snare Drum 2
const NoiseDrum::Effect NoiseDrum::effect006[] =
{
	16 * INTERVAL60, 0, 15 * FREQUENCY_SCALE, 55, 16, 3000 * INTERVAL, 0, 0, 15 * INTERVAL60, 1 * FREQUENCY_SCALE,
};

// Hi-Hat Close
const NoiseDrum::Effect NoiseDrum::effect007[] =
{
	6 * INTERVAL60, 39 * FREQUENCY_SCALE, 1, 54, 16, 500 * INTERVAL, 0, 0, 0, 0,
};

// Hi-Hat Open
const NoiseDrum::Effect NoiseDrum::effect008[] =
{
	32 * INTERVAL60, 39 * FREQUENCY_SCALE, 1, 54, 16, 5000 * INTERVAL, 0, 0, 0, 0,
};

// Crush Cymbal
const NoiseDrum::Effect NoiseDrum::effect009[] =
{
	31 * INTERVAL60, 40 * FREQUENCY_SCALE, 31 * FREQUENCY_SCALE, 54, 16, 5000 * INTERVAL, 0, 0, 15 * INTERVAL60, 1 * FREQUENCY_SCALE,
};

// Ride Cymbal
const NoiseDrum::Effect NoiseDrum::effect010[] =
{
	31 * INTERVAL60, 30 * FREQUENCY_SCALE, 1, 54, 16, 5000 * INTERVAL, 0, 0, 0, 0,
};

// 音データテーブル
const NoiseDrum::EffectData NoiseDrum::effectDatas[] =
{
	{sizeof(NoiseDrum::effect000) / sizeof(Effect), NoiseDrum::effect000},
	{sizeof(NoiseDrum::effect001) / sizeof(Effect), NoiseDrum::effect001},
	{sizeof(NoiseDrum::effect002) / sizeof(Effect), NoiseDrum::effect002},
	{sizeof(NoiseDrum::effect003) / sizeof(Effect), NoiseDrum::effect003},
	{sizeof(NoiseDrum::effect004) / sizeof(Effect), NoiseDrum::effect004},
	{sizeof(NoiseDrum::effect005) / sizeof(Effect), NoiseDrum::effect005},
	{sizeof(NoiseDrum::effect006) / sizeof(Effect), NoiseDrum::effect006},
	{sizeof(NoiseDrum::effect007) / sizeof(Effect), NoiseDrum::effect007},
	{sizeof(NoiseDrum::effect008) / sizeof(Effect), NoiseDrum::effect008},
	{sizeof(NoiseDrum::effect009) / sizeof(Effect), NoiseDrum::effect009},
	{sizeof(NoiseDrum::effect010) / sizeof(Effect), NoiseDrum::effect010}
};

// 変数
unsigned short NoiseDrum::rndSeed = 0;

unsigned char NoiseDrum::Rnd(void)
{
	unsigned short hl = NoiseDrum::rndSeed;
	unsigned short de = hl;
	hl += hl;
	hl += hl;
	hl += de;
	hl += 0x3711;
	NoiseDrum::rndSeed = hl;
	return hl >> 8;
}

NoiseDrum::NoiseDrum(void)
:counter(0)
,playIndex(0)
,effectData(NULL)
,nextPlayIndex(0)
,nextEffectData(NULL)
,phase(2)
,volume(0)
,noiseInterval(0)
,noiseReleaseCounter(0)
,noiseBeforeData(0)
,noiseSweepCounter(0)
,toneIntervalHalf(0)
,toneInterval(0)
,toneCounter(0)
,toneSweepCounter(0)
{
}

NoiseDrum::~NoiseDrum(void)
{
}

void NoiseDrum::SetPlay(uint8_t index)
{
	this->nextEffectData = &NoiseDrum::effectDatas[index];
	this->nextPlayIndex = 0;
	this->phase = 3;
}

void NoiseDrum::SetVolume(uint8_t volume)
{
	this->volume = volume;
}

void NoiseDrum::InitializePhase(void)
{
	this->toneInterval = this->effectData->data[this->playIndex].toneFrequency;
	this->toneIntervalHalf = this->toneInterval >> 1;
	this->noiseInterval = this->effectData->data[this->playIndex].noiseFrequency;
	this->noiseReleaseCounter = 0;
	this->toneSweepCounter = 0;
	this->noiseSweepCounter = 0;
	this->phase = 1;
}

void NoiseDrum::NextData(void)
{
	if(this->playIndex < this->effectData[this->playIndex].dataCount - 1)
	{
		++ this->playIndex;
		this->phase = 0;
		return;
	}
	this->phase = 2;
}

uint8_t NoiseDrum::GetData(void)
{
	if((this->effectData == NULL) && (this->nextEffectData == NULL))
	{
		return 0;
	}
	if(this->phase == 2)
	{
		return 0;
	}
	if(this->phase == 3)
	{
		this->playIndex = this->nextPlayIndex;
		this->effectData = this->nextEffectData;
		this->phase = 0;
	}
	if(this->phase == 0)
	{
		this->InitializePhase();
	}
	uint8_t data = 0;
	this->noiseReleaseCounter += INTERVAL;
	// Tone
	if((this->effectData->data[this->playIndex].mixControl & 1) == 0)
	{
		// Tone sweep
		this->toneSweepCounter += INTERVAL;
		if((this->effectData->data[this->playIndex].toneSweep > 0) && (this->toneSweepCounter > FPS60_INTERVAL))
		{
			this->toneInterval += this->effectData->data[this->playIndex].toneSweep;
			this->toneIntervalHalf = this->toneInterval >> 1;
			this->toneSweepCounter -= FPS60_INTERVAL;
		}
		this->toneCounter += INTERVAL;
		if(this->toneCounter < this->toneIntervalHalf)
		{
			data = 1;
		}
		else if(this->toneCounter > this->toneInterval)
		{
			this->toneCounter -= this->toneInterval;
			data = 1;
		}
		else
		{
			return 0;
		}
	}
	// Noise
	if((this->effectData->data[this->playIndex].mixControl & 8) == 0)
	{
		// Noise sweep
		this->noiseSweepCounter += INTERVAL;
		if((this->effectData->data[this->playIndex].noiseSweepCount > 0) && (this->noiseSweepCounter > this->effectData->data[this->playIndex].noiseSweepCount))
		{
			this->noiseInterval += this->effectData->data[this->playIndex].noiseSweepData;
			this->noiseSweepCounter -= this->effectData->data[this->playIndex].noiseSweepCount;
		}
		this->counter += INTERVAL;
		if(this->counter >= this->noiseInterval)
		{
			// 音量計算
			int noise = 0;
			if(this->effectData->data[this->playIndex].envelopeFrequency > 0)
			{
				if(this->noiseReleaseCounter < this->effectData->data[this->playIndex].envelopeFrequency)
				{
					noise = 1;
				}
			}
			else
			{
				noise = 1;
			}
			data = Rnd() < 128 ? 0 : noise;
			this->noiseBeforeData = data;
			this->counter -= this->noiseInterval;
		}
		else
		{
			data = this->noiseBeforeData;
		}
	}
	if(data == 1)
	{
		if(this->effectData->data[this->playIndex].envelopeFrequency == 0)
		{
			if(this->noiseReleaseCounter > this->effectData->data[this->playIndex].time)
			{
				this->NextData();
			}
			return volumeTable[this->effectData->data[this->playIndex].volume];
		}
		if(this->noiseReleaseCounter > this->effectData->data[this->playIndex].envelopeFrequency)
		{
			// エンベロープ終了だったら音量0
			this->NextData();
			return 0;
		}
		// 線形補完
		uint8_t volume = this->effectData->data[this->playIndex].volume - this->effectData->data[this->playIndex].volume * this->noiseReleaseCounter / this->effectData->data[this->playIndex].envelopeFrequency;
		volume = volume * this->volume / 16;
		volume = volumeTable[volume];
		return volume;
	}
	return 0;
}
