#include "apu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){TbaApu a;uint8_t r[0x30]={0},w[16]={0};unsigned int i,edges=0,total=0;int16_t prev=0;
    tba_apu_reset(&a);r[1]=0x80;r[2]=0xf0;r[3]=1547&255;r[4]=0x80|(1547>>8);r[20]=0x77;r[21]=0x11;
    tba_apu_snapshot(&a,r,w,0x81);
    for(i=0;i<TBA_RATE;i++){int16_t s=tba_apu_sample(&a);if(s>0&&prev<=0)edges++;prev=s;}
    assert(edges>=260&&edges<=264);
    r[4]&=0x7f;tba_apu_snapshot(&a,r,w,0x80);assert(!a.channel[0].enabled);
    tba_apu_reset(&a);for(i=0;i<10000;i++)total+=tba_apu_frame_samples(&a);
    assert(total==(uint32_t)((70224ULL*TBA_RATE*10000)>>22));
    /* Each channel must trigger from its own NRx4 register, with no cross-trigger. */
    for(unsigned int ch=0;ch<4;ch++){
        static const unsigned int base[4]={0,5,10,15};unsigned int b=base[ch];
        tba_apu_reset(&a);memset(r,0,sizeof(r));r[b]=0x80;r[b+2]=0xf0;r[b+4]=0x80;r[20]=0x77;r[21]=0xff;
        tba_apu_snapshot(&a,r,w,0x80|(1<<ch));for(i=0;i<4;i++)assert(a.channel[i].enabled==(i==ch));
    }
    /* Envelope falls on the 64 Hz clock; disabling DAC silences immediately. */
    tba_apu_reset(&a);memset(r,0,sizeof(r));r[2]=0xf1;r[4]=0x80;tba_apu_snapshot(&a,r,w,0x81);
    for(i=0;i<TBA_RATE/2;i++)tba_apu_sample(&a);assert(a.channel[0].volume==0);
    r[2]=0;r[4]=0;tba_apu_snapshot(&a,r,w,0x81);assert(!a.channel[0].enabled);
    puts("Pulse pitch, channel routing/triggers, envelope, DAC disable and frame sample clock passed.");
}
