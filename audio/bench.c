#include "apu.h"
unsigned int tba_bench_setting=0x35;
static TbaApu a;
static int16_t out[268];
void tba_bench_init(void){uint8_t r[0x30]={0},w[16]={0};tba_apu_reset(&a);
    r[1]=0x80;r[2]=0xf3;r[3]=11;r[4]=0x86;
    r[6]=0x80;r[7]=0xa3;r[8]=80;r[9]=0x85;
    r[10]=0x80;r[12]=0x20;r[13]=128;r[14]=0x85;
    r[16]=0;r[17]=0xc0;r[18]=tba_bench_setting;r[19]=0x80;
    r[20]=0x77;r[21]=0xff;for(unsigned int i=0;i<16;i++)w[i]=i*17;
    tba_apu_snapshot(&a,r,w,0x8f);
#ifdef __ez80__
*(unsigned int*)0xd17000=(unsigned int)out;
#endif

}
void tba_bench_frame(void){tba_apu_render(&a,out,268);}

#ifndef __ez80__
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){if(argc>1)tba_bench_setting=strtoul(argv[1],0,0);tba_bench_init();for(unsigned int i=0;i<120;i++){tba_bench_frame();fwrite(out,2,268,stdout);}return 0;}
#endif
