#include "PsgDrum.h"
#include <string.h>

namespace psg_detail {


const uint8_t SequencedVoice::volumeTable[] =
{
	0, 10, 12, 16, 20, 25, 32, 40, 51, 64, 80, 101, 128, 161, 203, 255, 255
};
// Bass Drum
const SequencedVoice::Effect SequencedVoice::effect000[] =
{
	1 * INTERVAL60, 1500 * FREQUENCY_SCALE, 31 * FREQUENCY_SCALE, 54, 15, 0, 0, 127 * FREQUENCY_SCALE, 0, 0,
	8 * INTERVAL60, 1700 * FREQUENCY_SCALE, 1, 62, 16, 1200 * INTERVAL, 0, 127 * FREQUENCY_SCALE, 0, 0,
};
// Snare Drum
const SequencedVoice::Effect SequencedVoice::effect001[] =
{
	14 * INTERVAL60, 400 * FREQUENCY_SCALE, 7 * FREQUENCY_SCALE, 54, 16, 3000 * INTERVAL, 0, 93 * FREQUENCY_SCALE, 15 * INTERVAL60, 2 * FREQUENCY_SCALE,
};
// Low Tom
const SequencedVoice::Effect SequencedVoice::effect002[] =
{
	2 * INTERVAL60, 700 * FREQUENCY_SCALE, 1, 54, 15, 0, 0, 100 * FREQUENCY_SCALE, 0, 0,
	14 * INTERVAL60, 900 * FREQUENCY_SCALE, 1, 54, 16, 2500 * INTERVAL, 0, 100 * FREQUENCY_SCALE, 0, 0,
};
// Middle Tom
const SequencedVoice::Effect SequencedVoice::effect003[] =
{
	2 * INTERVAL60, 500 * FREQUENCY_SCALE, 5 * FREQUENCY_SCALE, 54, 15, 0, 0, 60 * FREQUENCY_SCALE, 0, 0,
	14 * INTERVAL60, 620 * FREQUENCY_SCALE, 1, 54, 16, 2500 * INTERVAL, 0, 60 * FREQUENCY_SCALE, 0, 0,
};
// High Tom
const SequencedVoice::Effect SequencedVoice::effect004[] =
{
	2 * INTERVAL60, 300 * FREQUENCY_SCALE, 1, 54, 15, 0, 0, 50 * FREQUENCY_SCALE, 0, 0,
	14 * INTERVAL60, 400 * FREQUENCY_SCALE, 1, 54, 16, 2500 * INTERVAL, 0, 50 * FREQUENCY_SCALE, 0, 0,
};
// Rim Shot
const SequencedVoice::Effect SequencedVoice::effect005[] =
{
	2 * INTERVAL60, 55 * FREQUENCY_SCALE, 1, 62, 16, 300 * INTERVAL, 0, 100 * FREQUENCY_SCALE, 0, 0,
};
// Snare Drum 2
const SequencedVoice::Effect SequencedVoice::effect006[] =
{
	16 * INTERVAL60, 0, 15 * FREQUENCY_SCALE, 55, 16, 3000 * INTERVAL, 0, 0, 15 * INTERVAL60, 1 * FREQUENCY_SCALE,
};
// Hi-Hat Close
const SequencedVoice::Effect SequencedVoice::effect007[] =
{
	6 * INTERVAL60, 39 * FREQUENCY_SCALE, 1, 54, 16, 500 * INTERVAL, 0, 0, 0, 0,
};
// Hi-Hat Open
const SequencedVoice::Effect SequencedVoice::effect008[] =
{
	32 * INTERVAL60, 39 * FREQUENCY_SCALE, 1, 54, 16, 5000 * INTERVAL, 0, 0, 0, 0,
};
// Crush Cymbal
const SequencedVoice::Effect SequencedVoice::effect009[] =
{
	31 * INTERVAL60, 40 * FREQUENCY_SCALE, 31 * FREQUENCY_SCALE, 54, 16, 5000 * INTERVAL, 0, 0, 15 * INTERVAL60, 1 * FREQUENCY_SCALE,
};
// Ride Cymbal
const SequencedVoice::Effect SequencedVoice::effect010[] =
{
	31 * INTERVAL60, 30 * FREQUENCY_SCALE, 1, 54, 16, 5000 * INTERVAL, 0, 0, 0, 0,
};
// 音データテーブル
const SequencedVoice::EffectData SequencedVoice::effectDatas[] =
{
	{sizeof(SequencedVoice::effect000) / sizeof(Effect), SequencedVoice::effect000},
	{sizeof(SequencedVoice::effect001) / sizeof(Effect), SequencedVoice::effect001},
	{sizeof(SequencedVoice::effect002) / sizeof(Effect), SequencedVoice::effect002},
	{sizeof(SequencedVoice::effect003) / sizeof(Effect), SequencedVoice::effect003},
	{sizeof(SequencedVoice::effect004) / sizeof(Effect), SequencedVoice::effect004},
	{sizeof(SequencedVoice::effect005) / sizeof(Effect), SequencedVoice::effect005},
	{sizeof(SequencedVoice::effect006) / sizeof(Effect), SequencedVoice::effect006},
	{sizeof(SequencedVoice::effect007) / sizeof(Effect), SequencedVoice::effect007},
	{sizeof(SequencedVoice::effect008) / sizeof(Effect), SequencedVoice::effect008},
	{sizeof(SequencedVoice::effect009) / sizeof(Effect), SequencedVoice::effect009},
	{sizeof(SequencedVoice::effect010) / sizeof(Effect), SequencedVoice::effect010}
};
// 変数
unsigned short SequencedVoice::rndSeed = 0;
unsigned char SequencedVoice::Rnd(void)
{
	unsigned short hl = SequencedVoice::rndSeed;
	unsigned short de = hl;
	hl += hl;
	hl += hl;
	hl += de;
	hl += 0x3711;
	SequencedVoice::rndSeed = hl;
	return hl >> 8;
}
SequencedVoice::SequencedVoice(void)
:counter(0)
,playIndex(0)
,effectData(NULL)
,nextPlayIndex(0)
,nextEffectData(NULL)
,phase(2)
,volume(0)
,masterVolume(0)
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
SequencedVoice::~SequencedVoice(void)
{
}
void SequencedVoice::SetPlay(uint8_t index, uint8_t volume)
{
    if(index >= EFFECT_COUNT) return;
	this->nextEffectData = &SequencedVoice::effectDatas[index];
	this->nextPlayIndex = 0;
	this->volume = volume > 15 ? 15 : volume;
	this->phase = 3;
}
void SequencedVoice::SetVolume(uint8_t volume)
{
	this->masterVolume = volume > 15 ? 15 : volume;
}
void SequencedVoice::InitializePhase(void)
{
	this->toneInterval = this->effectData->data[this->playIndex].toneFrequency;
	this->toneIntervalHalf = this->toneInterval >> 1;
	this->noiseInterval = this->effectData->data[this->playIndex].noiseFrequency;
	this->noiseReleaseCounter = 0;
	this->toneSweepCounter = 0;
	this->noiseSweepCounter = 0;
	this->phase = 1;
}
void SequencedVoice::NextData(void)
{
	if(this->playIndex < this->effectData->dataCount - 1)
	{
		++ this->playIndex;
		this->phase = 0;
		return;
	}
	this->phase = 2;
}
uint8_t SequencedVoice::GetData(void)
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
    // Advance even on a zero-valued noise sample. Otherwise noise-only
    // effects can remain active forever after their envelope has ended.
    const auto& effect = this->effectData->data[this->playIndex];
    const uint32_t duration = effect.envelopeFrequency ? effect.envelopeFrequency : effect.time;
    if(this->noiseReleaseCounter > duration) { this->NextData(); return 0; }
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
			data = static_cast<uint8_t>(Rnd() < 128 ? 0 : noise);
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
			return static_cast<uint8_t>(volumeTable[this->effectData->data[this->playIndex].volume] * this->volume * this->masterVolume / 225u);
		}
		if(this->noiseReleaseCounter > this->effectData->data[this->playIndex].envelopeFrequency)
		{
			// エンベロープ終了だったら音量0
			this->NextData();
			return 0;
		}
		// 線形補完
		uint8_t volume = static_cast<uint8_t>(this->effectData->data[this->playIndex].volume - this->effectData->data[this->playIndex].volume * this->noiseReleaseCounter / this->effectData->data[this->playIndex].envelopeFrequency);
		volume = static_cast<uint8_t>(volume * this->volume * this->masterVolume / 256);
		volume = volumeTable[volume];
		return volume;
	}
	return 0;
}



const ProceduralVoice::Preset ProceduralVoice::presets[31] = {
        // note, square Hz x2, noise Hz, duration ms, pitch drop/rise, pulse, count, tone blend /255, peak
        {39,0,0,11000,150,0,24,3,0,80}, // Hand clap: three noise bursts
        {44,5700,8100,12000,65,0,0,0,65,48}, // Pedal hi-hat
        {52,1850,2710,10000,480,-300,0,0,95,56}, // Chinese cymbal
        {53,1800,2630,0,340,0,0,0,255,48}, // Ride bell
        {54,4100,5930,11000,230,0,38,5,100,52}, // Tambourine
        {55,3500,5270,12000,220,-450,0,0,70,54}, // Splash
        {56,560,845,0,180,0,0,0,255,64}, // Cowbell
        {58,780,1130,4500,400,-200,36,9,90,60}, // Vibraslap
        {59,2900,4210,10500,520,0,0,0,100,48}, // Ride cymbal 2
        {60,700,0,0,100,-330,0,0,255,72}, // High bongo
        {61,480,0,0,140,-240,0,0,255,76}, // Low bongo
        {62,540,0,0,70,-210,0,0,255,66}, // Muted high conga
        {63,480,0,0,190,-190,0,0,255,72}, // Open high conga
        {64,340,0,0,240,-150,0,0,255,76}, // Low conga
        {65,1150,1720,0,120,-380,0,0,255,60}, // High timbale
        {66,800,1190,0,170,-270,0,0,255,64}, // Low timbale
        {67,1050,1540,0,200,0,0,0,255,58}, // High agogo
        {68,730,1080,0,240,0,0,0,255,62}, // Low agogo
        {69,0,0,8500,115,0,18,5,0,62}, // Cabasa
        {70,0,0,12000,85,0,0,0,0,66}, // Maracas
        {71,2300,0,0,110,100,0,0,255,40}, // Short whistle
        {72,2000,0,0,550,350,0,0,255,40}, // Long whistle
        {73,0,0,3200,130,0,15,7,0,64}, // Short guiro
        {74,0,0,2800,400,0,22,15,0,60}, // Long guiro
        {75,2450,0,0,45,0,0,0,255,64}, // Claves
        {76,1450,0,0,75,-170,0,0,255,70}, // High wood block
        {77,950,0,0,95,-140,0,0,255,74}, // Low wood block
        {78,850,0,0,95,700,0,0,255,50}, // Muted cuica
        {79,480,0,0,240,650,0,0,255,54}, // Open cuica
        {80,3100,4384,0,75,0,0,0,255,42}, // Muted triangle
        {81,3100,4384,0,750,0,0,0,255,42}, // Open triangle
    };

const ProceduralVoice::Preset* ProceduralVoice::find(uint8_t note) {
    for(const auto& p : presets) if(p.note==note) return &p;
    return nullptr;
}

bool ProceduralVoice::playing() const { return remaining!=0; }

void ProceduralVoice::stop() { remaining=0; }

void ProceduralVoice::start(const Preset& p) {
    remaining=uint32_t(p.milliseconds)*48;
    phase1=phase2=noise_phase=0;
    step1=step(p.tone1); step2=step(p.tone2); noise_step=step(p.noise);
    const uint32_t ticks=(remaining+31)/32;
    sweep=static_cast<int32_t>((int64_t(p.sweep_hz)*4294967296ll/48000)/ticks);
    pulse_frames=uint32_t(p.pulse_ms)*48; pulse_left=pulse_frames;
    pulses_left=p.pulses ? p.pulses-1 : 0;
    const uint32_t decay_ticks=pulse_frames ? (pulse_frames+31)/32 : ticks;
    drop=(65535+decay_ticks-1)/decay_ticks;
    envelope=65535; tick=0; blend=p.tone_mix; peak=p.peak;
    rng=0x9e3779b9u ^ (uint32_t(p.note)*0x10001u); noise_value=1;
}

int16_t ProceduralVoice::sample() {
    if(!remaining) return 0;
    --remaining;
    if(pulses_left && --pulse_left==0) {
        --pulses_left; pulse_left=pulse_frames; envelope=65535;
    }
    phase1+=step1; phase2+=step2;
    const uint32_t old=noise_phase; noise_phase+=noise_step;
    if(noise_phase<old) {
        rng^=rng<<13; rng^=rng>>17; rng^=rng<<5;
        noise_value=(rng&1) ? 1 : -1;
    }
    const int tone=(phase1&0x80000000u) ? 1 : -1;
    const int second=(phase2&0x80000000u) ? 1 : -1;
    const int tonal=step2 ? (tone+second)*127 : tone*254;
    const int mixed=(tonal*blend+noise_value*254*(255-blend))>>8;
    // Squared decay, updated at 1.5 kHz. No division in the oscillator updates.
    const uint32_t level=(envelope*envelope)>>16;
    const int16_t result=static_cast<int16_t>((mixed*int32_t(peak)*int32_t(level>>8))>>16);
    if(++tick==32) {
        tick=0; envelope=envelope>drop ? envelope-drop : 0;
        if(step1) step1=static_cast<uint32_t>(static_cast<int32_t>(step1)+sweep);
    }
    return result;
}

uint32_t ProceduralVoice::step(uint16_t hz) { return static_cast<uint32_t>((uint64_t(hz)<<32)/48000); }

}

void PsgDrum::stop(int ch) {
    for(auto& d : sequenced) if(ch < 0 || d.channel == ch) { d.engine.Stop(); d.sample = 0; d.clock = 0; }
    for(auto& d : procedural) if(ch < 0 || d.channel == ch) d.engine.stop();
}
unsigned PsgDrum::active_count(int ch) const {
    unsigned count = 0;
    for(const auto& d : sequenced) if(d.engine.IsPlaying() && (ch < 0 || d.channel == ch)) ++count;
    for(const auto& d : procedural) if(d.engine.playing() && (ch < 0 || d.channel == ch)) ++count;
    return count;
}
void PsgDrum::update_gain(uint8_t ch, uint16_t gain, uint8_t volume, uint8_t expression) {
    for(auto& d : sequenced) if(d.channel == ch)
        d.engine.SetVolume(static_cast<uint8_t>((volume*expression*4u)>>12));
    for(auto& d : procedural) if(d.channel == ch)
        d.gain = static_cast<uint16_t>(uint32_t(gain)*d.velocity/127);
}
// Return true when an active voice had to be replaced.
bool PsgDrum::start(uint8_t ch, uint8_t note, uint8_t velocity,
                    uint16_t gain, uint8_t volume, uint8_t expression) {
    if(note < 35 || note > 81 || !velocity) return false;
    bool stolen = false;
    // Three tom pitches: floor 41/43=low, 45=middle, 47/48/50=high.
    // In particular, the 47 -> 45 -> 43 fill must descend (255=unassigned).
    static const uint8_t map[] = {0,0,5,1,255,6,2,7,2,255,3,8,4,4,9,4,10,255,255,255,255,255,9};
    if(note > 57 || map[note-35] == 255) {
        const auto* preset = psg_detail::ProceduralVoice::find(note);
        if(!preset) return stolen;
        ProceduralSlot* selected = nullptr;
        // Retrigger only the same added note/channel; mute/open triangles share a group.
        for(auto& d : procedural) if(d.engine.playing() && d.channel == ch &&
            (d.note == note || (note >= 80 && d.note >= 80))) {
            d.engine.stop(); selected = &d;
        }
        if(!selected) for(auto& d : procedural) if(!d.engine.playing()) { selected = &d; break; }
        if(!selected) {
            selected = &procedural[0];
            for(auto& d : procedural)
                if(uint32_t(serial-d.serial) > uint32_t(serial-selected->serial)) selected = &d;
            stolen = true;
        }
        selected->channel = ch; selected->note = note; selected->velocity = velocity;
        selected->serial = ++serial;
        selected->gain = static_cast<uint16_t>(uint32_t(gain)*velocity/127);
        selected->engine.start(*preset);
        return stolen;
    }
    auto& d = sequenced[map[note-35]];
    d.channel = ch; d.clock = 0; d.sample = 0;
    d.engine.SetPlay(map[note-35],velocity>>3);
    d.engine.SetVolume(static_cast<uint8_t>((volume*expression*4u)>>12));
    return stolen;

}
int32_t PsgDrum::sample(uint16_t master) {
    int32_t mix = 0;
    // Preserve shared RNG order and the 32 -> 48 kHz sample hold.
    for(auto& d : sequenced) {
        if(!d.engine.IsPlaying()) { d.sample = 0; continue; }
        d.clock += 32000;
        if(d.clock >= 48000) { d.clock -= 48000; d.sample = d.engine.GetData(); }
        mix += static_cast<int32_t>(d.sample)*128*master>>14;
    }
    for(auto& d : procedural)
        if(d.engine.playing()) mix += int32_t(d.engine.sample())*d.gain/128;
    return mix*9/5;
}
