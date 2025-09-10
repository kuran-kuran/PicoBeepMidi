#include <string.h>
#include "NoiseDrum.hpp"

static const uint8_t volumeTable[] =
{
    0, 10, 12, 16, 20, 25, 32, 40, 51, 64, 80, 101, 128, 161, 203, 255, 255
};

// Bass Drum
static const Effect effect000[] =
{
    1 * INTERVAL60, 1500 * FREQUENCY_SCALE, 31 * FREQUENCY_SCALE, 54, 15, 0, 0, 127 * FREQUENCY_SCALE, 0, 0,
    8 * INTERVAL60, 1700 * FREQUENCY_SCALE, 1, 62, 16, 1200 * INTERVAL, 0, 127 * FREQUENCY_SCALE, 0, 0,
};

// Snare Drum
static const Effect effect001[] =
{
    14 * INTERVAL60, 400 * FREQUENCY_SCALE, 7 * FREQUENCY_SCALE, 54, 16, 3000 * INTERVAL, 0, 93 * FREQUENCY_SCALE, 15 * INTERVAL60, 2 * FREQUENCY_SCALE,
};

// Low Tom
static const Effect effect002[] =
{
    2 * INTERVAL60, 700 * FREQUENCY_SCALE, 1, 54, 15, 0, 0, 100 * FREQUENCY_SCALE, 0, 0,
    14 * INTERVAL60, 900 * FREQUENCY_SCALE, 1, 54, 16, 2500 * INTERVAL, 0, 100 * FREQUENCY_SCALE, 0, 0,
};

// Middle Tom
static const Effect effect003[] =
{
    2 * INTERVAL60, 500 * FREQUENCY_SCALE, 5 * FREQUENCY_SCALE, 54, 15, 0, 0, 60 * FREQUENCY_SCALE, 0, 0,
    14 * INTERVAL60, 620 * FREQUENCY_SCALE, 1, 54, 16, 2500 * INTERVAL, 0, 60 * FREQUENCY_SCALE, 0, 0,
};

// High Tom
static const Effect effect004[] =
{
    2 * INTERVAL60, 300 * FREQUENCY_SCALE, 1, 54, 15, 0, 0, 50 * FREQUENCY_SCALE, 0, 0,
    14 * INTERVAL60, 400 * FREQUENCY_SCALE, 1, 54, 16, 2500 * INTERVAL, 0, 50 * FREQUENCY_SCALE, 0, 0,
};

// Rim Shot
static const Effect effect005[] =
{
    2 * INTERVAL60, 55 * FREQUENCY_SCALE, 1, 62, 16, 300 * INTERVAL, 0, 100 * FREQUENCY_SCALE, 0, 0,
};

// Snare Drum 2
static const Effect effect006[] =
{
    16 * INTERVAL60, 0, 15 * FREQUENCY_SCALE, 55, 16, 3000 * INTERVAL, 0, 0, 15 * INTERVAL60, 1 * FREQUENCY_SCALE,
};

// Hi-Hat Close
static const Effect effect007[] =
{
    6 * INTERVAL60, 39 * FREQUENCY_SCALE, 1, 54, 16, 500 * INTERVAL, 0, 0, 0, 0,
};

// Hi-Hat Open
static const Effect effect008[] =
{
    32 * INTERVAL60, 39 * FREQUENCY_SCALE, 1, 54, 16, 5000 * INTERVAL, 0, 0, 0, 0,
};

// Crush Cymbal
static const Effect effect009[] =
{
    31 * INTERVAL60, 40 * FREQUENCY_SCALE, 31 * FREQUENCY_SCALE, 54, 16, 5000 * INTERVAL, 0, 0, 15 * INTERVAL60, 1 * FREQUENCY_SCALE,
};

// Ride Cymbal
static const Effect effect010[] =
{
    31 * INTERVAL60, 30 * FREQUENCY_SCALE, 1, 54, 16, 5000 * INTERVAL, 0, 0, 0, 0,
};

const EffectData effectDatas[] =
{
    {sizeof(effect000) / sizeof(Effect), effect000},
    {sizeof(effect001) / sizeof(Effect), effect001},
    {sizeof(effect002) / sizeof(Effect), effect002},
    {sizeof(effect003) / sizeof(Effect), effect003},
    {sizeof(effect004) / sizeof(Effect), effect004},
    {sizeof(effect005) / sizeof(Effect), effect005},
    {sizeof(effect006) / sizeof(Effect), effect006},
    {sizeof(effect007) / sizeof(Effect), effect007},
    {sizeof(effect008) / sizeof(Effect), effect008},
    {sizeof(effect009) / sizeof(Effect), effect009},
    {sizeof(effect010) / sizeof(Effect), effect010}
};

// 変数

unsigned char NoiseDrum::Rnd(void)
{
    unsigned short hl = rndSeed;
    unsigned short de = hl;
    hl += hl;
    hl += hl;
    hl += de;
    hl += 0x3711;
    rndSeed = hl;
    return hl >> 8;
}

void NoiseDrum::NoiseDrum(void)
{
}

void NoiseDrum::~NoiseDrum(void)
{
}

void NoiseDrum::SetPlay(uint8_t index)
{
    this->effectData = &effectDatas[index];
    this->playIndex = 0;
    this->phase = 0;
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
    if(this->playIndex < this->effectData[this->playIndex].dataCount)
    {
        ++ this->playIndex;
        this->phase = 0;
        return;
    }
    this->phase = 2;
}

uint8_t NoiseDrum::GetData(void)
{
    if(this->phase == 2)
    {
        return 0;
    }
    if(this->phase == 0)
    {
        NoiseDrumInitializePhase(drum);
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
                NoiseDrumNextData(drum);
            }
            return volumeTable[this->effectData->data[this->playIndex].volume];
        }
        if(this->noiseReleaseCounter > this->effectData->data[this->playIndex].envelopeFrequency)
        {
            // エンベロープ終了だったら音量0
            NoiseDrumNextData(drum);
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
