#include "apu.h"
#include <string.h>
#ifdef __ez80__
unsigned int tba_phase[4],tba_step[4],tba_noise;
typedef struct __attribute__((packed)) {int value;uint8_t pad;} FastLevel;
_Static_assert(sizeof(FastLevel)==4,"Assembly table cell layout");
FastLevel tba_levels[4][64] __attribute__((aligned(256)));
_Static_assert(sizeof(tba_levels[0])==256,"Assembly table row layout");
uint8_t tba_short;
static int cache_gain[4];
static uint8_t cache_duty[2],cache_wave[16],cache_shift;
void tba_fast_mix(int16_t *out,unsigned int n);
#endif
static const uint8_t offsets[4]={0,5,10,15};
static const uint8_t duties[4]={0x01,0x81,0x87,0x7e};
static uint32_t step(unsigned int frequency, uint8_t wave) {
    /* Q16 sequencer positions per 16 kHz sample; divisions are frame-only. */
    return (wave ? 8589934UL : 4294967UL)/(2048-frequency);
}
void tba_apu_reset(TbaApu *a) { memset(a,0,sizeof(*a)); a->lfsr=0x7fff;
#ifdef __ez80__
memset(tba_phase,0,sizeof(tba_phase));tba_noise=0x7fff;
for(unsigned int i=0;i<4;i++)cache_gain[i]=-1;
#endif
}
static void envelope(TbaChannel *c,uint8_t value) {
    c->volume=value>>4; c->envelope_up=!!(value&8);
    c->envelope_period=value&7; c->envelope_ticks=c->envelope_period;
}
void tba_apu_snapshot(TbaApu *a,const uint8_t regs[0x30],const uint8_t wave[16],uint8_t status) {
    unsigned int i;
    memcpy(a->regs,regs,0x30);memcpy(a->wave,wave,16);
    if(!(status&0x80)){for(i=0;i<4;i++)a->channel[i].enabled=0;return;}
    for(i=0;i<4;i++) {
        TbaChannel *c=&a->channel[i];const uint8_t *r=regs+offsets[i];
        uint8_t trigger=r[4]&0x80;
        if(!(status&(1<<i)))c->enabled=0;
        if(i<3) {
            unsigned int f=r[3]|((r[4]&7)<<8);
            if(trigger||f!=c->shadow_frequency){c->shadow_frequency=f;c->frequency=f;c->step=step(f,i==2);}
        } else {
            static const uint8_t divs[8]={8,16,32,48,64,80,96,112};
            a->channel[3].step=17179869UL/((uint32_t)divs[r[3]&7]<<(r[3]>>4));
        }
        if(trigger) {
            c->enabled=!!(status&(1<<i));
            /* Pulse retriggers reset envelope and sweep, but preserve duty phase. */
            if(i==2){c->phase=0;c->length=256-r[1];
#ifdef __ez80__
tba_phase[2]=0;
#endif
}
            else {c->length=64-(r[1]&63);envelope(c,r[2]);}
            if(i==0){a->sweep_period=(r[0]>>4)&7;a->sweep_ticks=a->sweep_period?a->sweep_period:8;a->sweep_shift=r[0]&7;a->sweep_down=!!(r[0]&8);}
            if(i==3){a->lfsr=0x7fff;c->phase=0;
#ifdef __ez80__
tba_noise=0x7fff;tba_phase[3]=0;
#endif
}
        }
        if(i==2?!(r[0]&0x80):!(r[2]&0xf8))c->enabled=0;
    }
}
static void sequencer(TbaApu *a) {
    unsigned int i;uint8_t frame=a->sequencer;
    if(!(frame&1))for(i=0;i<4;i++) {
        TbaChannel *c=&a->channel[i];
        if((a->regs[offsets[i]+4]&0x40)&&c->length&&!--c->length)c->enabled=0;
    }
    if(frame==2||frame==6) {
        if(a->sweep_ticks&&!--a->sweep_ticks) {
            TbaChannel *c=&a->channel[0];unsigned int delta=c->frequency>>a->sweep_shift;
            a->sweep_ticks=a->sweep_period?a->sweep_period:8;
            if(a->sweep_shift&&a->sweep_period){
                unsigned int next=a->sweep_down?c->frequency-delta:c->frequency+delta;
                if(next>2047)c->enabled=0;
                else {c->frequency=next;c->step=step(next,0);if(!a->sweep_down&&next+(next>>a->sweep_shift)>2047)c->enabled=0;}
            }
        }
    }
    if(frame==7)for(i=0;i<4;i++)if(i!=2) {
        TbaChannel *c=&a->channel[i];
        if(c->envelope_period&&c->envelope_ticks&&!--c->envelope_ticks){
            c->envelope_ticks=c->envelope_period;
            if(c->envelope_up){if(c->volume<15)c->volume++;}else if(c->volume)c->volume--;
        }
    }
    a->sequencer=(frame+1)&7;
}
static int16_t mix_sample(TbaApu *a) {
    unsigned int i;int32_t mixed=0;
    for(i=0;i<4;i++) {
        TbaChannel *c=&a->channel[i];int value;
        c->phase+=c->step;
        if(i==3){unsigned int clocks=c->phase>>16;c->phase&=65535;
            while(clocks--){uint16_t bit=(a->lfsr^(a->lfsr>>1))&1;a->lfsr=(a->lfsr>>1)|(bit<<14);if(a->regs[18]&8)a->lfsr=(a->lfsr&~64)|(bit<<6);}
            value=(a->lfsr&1)?-(int)c->volume:c->volume;
        } else if(i==2) {
            unsigned int pos=(c->phase>>16)&31;uint8_t byte=a->wave[pos>>1],shift=(a->regs[12]>>5)&3;
            value=shift?((int)((pos&1)?byte&15:byte>>4)*2-15)>>(shift-1):0;
        } else value=(duties[a->regs[offsets[i]+1]>>6]&(1<<((c->phase>>16)&7)))?c->volume:-(int)c->volume;
        if(c->enabled){uint8_t routing=a->regs[21],volume=a->regs[20];int gain=0;
            if(routing&(1<<i))gain+=(volume&7)+1;
            if(routing&(16<<i))gain+=((volume>>4)&7)+1;
            mixed+=(int32_t)value*gain*16;
        }
    }
    /* DC blocking runs in the computer audio worklet, outside the emulator CPU. */
    if(mixed>32767)mixed=32767;if(mixed< -32768)mixed= -32768;
    return (int16_t)mixed;
}
unsigned int tba_apu_frame_samples(TbaApu *a) {
    a->sample_clock+=70224UL*TBA_RATE;
    unsigned int n=a->sample_clock>>22;a->sample_clock&=0x3fffff;return n;
}

int16_t tba_apu_sample(TbaApu *a) {
    int16_t s=mix_sample(a);a->tick+=512;
    if(a->tick>=TBA_RATE){a->tick-=TBA_RATE;sequencer(a);}return s;
}
void tba_apu_render(TbaApu *a,int16_t *out,unsigned int n) {
    unsigned int total=n;
#ifdef __ez80__
    {
        unsigned int i,p;
        for(i=0;i<4;i++){
            TbaChannel *c=&a->channel[i];unsigned int gain=0;
            if(c->enabled){if(a->regs[21]&(1<<i))gain+=(a->regs[20]&7)+1;if(a->regs[21]&(16<<i))gain+=((a->regs[20]>>4)&7)+1;}
            gain*=16;tba_step[i]=c->enabled?c->step:0;
            if(i==3&&!c->volume&&!c->envelope_up)tba_step[i]=0;
            int signature=i==2?(int)gain:(int)gain*c->volume;
            uint8_t duty=i<2?a->regs[offsets[i]+1]>>6:0;
            uint8_t shift=(a->regs[12]>>5)&3;
            if(cache_gain[i]==signature){
                if(i<2&&cache_duty[i]==duty)continue;
                if(i==3)continue;
                if(i==2&&cache_shift==shift&&!memcmp(cache_wave,a->wave,16))continue;
            }
            cache_gain[i]=signature;
            if(i<2)cache_duty[i]=duty;
            if(i==2){cache_shift=shift;memcpy(cache_wave,a->wave,16);}
            for(p=0;p<(i==2?32:i==3?2:8);p++){
                int value;
                if(i==2){uint8_t byte=a->wave[p>>1],shift=(a->regs[12]>>5)&3;value=shift?((int)((p&1)?byte&15:byte>>4)*2-15)>>(shift-1):0;}
                else if(i==3)value=p?-(int)c->volume:c->volume;
                else value=(duties[a->regs[offsets[i]+1]>>6]&(1<<p))?c->volume:-(int)c->volume;
                tba_levels[i][p].value=value*(int)gain;
            }
        }
        tba_short=!!(a->regs[18]&8);
        tba_fast_mix(out,n);
    }
#else
    while(n--)*out++=mix_sample(a);
#endif
    uint32_t clocks=a->tick+(uint32_t)total*512;
    while(clocks>=TBA_RATE){clocks-=TBA_RATE;sequencer(a);}
    a->tick=clocks;
}
