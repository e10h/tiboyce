/* ROM-free isolated eZ80 timing, using the CEmu CPU core. */
#include "emu.h"
#include "asic.h"
#include "cpu.h"
#include "mem.h"
#include "control.h"
#include "flash.h"
#include "bridge.h"
#include <assert.h>

#define CHECK_CHUNKS 120

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
void gui_console_clear(void){}
void gui_console_printf(const char *f,...){(void)f;}
void gui_console_err_printf(const char *f,...){(void)f;}
asic_rev_t gui_handle_reset(const boot_ver_t*v,asic_rev_t l,asic_rev_t d,emu_device_t dev,bool*python){(void)v;(void)l;(void)d;(void)dev;*python=false;return ASIC_REV_I;}

static void load(const char *path,unsigned address){FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);assert(fread(phys_mem_ptr(address,n),1,n,f)==(size_t)n);fclose(f);}
static void check(int z80){
    cpu_clear_signals();cpu.halted=false;cpu.ADL=cpu.L=cpu.IL=!z80;cpu.MADL=true;cpu.IEF1=cpu.IEF2=false;
    cpu.registers.AF=0xa5d7;cpu.registers._AF=0x5a29;
    cpu.registers.BC=0x123456;cpu.registers.DE=0x234567;cpu.registers.HL=0x345678;
    cpu.registers._BC=0x654321;cpu.registers._DE=0x765432;cpu.registers._HL=0x876543;
    cpu.registers.IX=0xabcdef;cpu.registers.IY=0xcdefab;cpu.registers.I=0x4791;cpu.registers.MBASE=0xd4;
    cpu.registers.SPL=0xd17f00;cpu.registers.SPS=0xfd00;
    eZ80registers_t before=cpu.registers;
    mem_poke_word(0xd17f00,0xd18020,true);mem_poke_byte(0xd18020,0x76);
    cpu_flush(z80?0x8000:AudioPoll,!z80);
    for(unsigned i=0;i<10000&&!cpu.halted;i++){sched.event.next_cycle=cpu.cycles+100;cpu_execute();}
    assert(cpu.halted);assert(cpu.ADL==!z80);
#define SAME(x) assert(cpu.registers.x==before.x)
    SAME(AF);SAME(_AF);SAME(BC);SAME(DE);SAME(HL);SAME(_BC);SAME(_DE);SAME(_HL);SAME(IX);SAME(IY);SAME(I);SAME(MBASE);SAME(SPS);
    assert(cpu.registers.SPL==before.SPL+(z80?0:3));
}
int main(int argc,char**argv){assert(argc==3);asic_init();mem.flash.size=0x400000;mem.flash.block=calloc(mem.flash.size,1);asic_reset();control.stackLimit=0;control.protectedStart=control.protectedEnd=0;
load(argv[1],0xd1a881);load(argv[2],0xd48000);check(0);check(1);puts("ADL and Z80 CALL.IL USB poll bridges preserve both register banks, I, IX/IY and stack modes.");return 0;}
