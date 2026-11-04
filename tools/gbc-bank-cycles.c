/* ROM-free isolated eZ80 timing, using the CEmu CPU core. */
#include "emu.h"
#include "asic.h"
#include "cpu.h"
#include "mem.h"
#include "control.h"
#include "flash.h"
#include "banks.h"
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
static void run(unsigned pc,bool adl) {
    cpu_clear_signals();cpu.halted=false;cpu.ADL=cpu.L=cpu.IL=adl;cpu.MADL=true;cpu.IEF1=cpu.IEF2=false;
    cpu.registers.MBASE=0xd4;cpu.registers.SPL=0xd17f00;cpu.registers.SPS=0xfdf0;
    mem_poke_word(0xd17f00,0xd18020,true);mem_poke_byte(0xd18020,0x76);
    mem_poke_word(0xd4fdf0,0x8000,false);mem_poke_byte(0xd48000,0x76);
    cpu_flush(pc,adl);
    for(unsigned i=0;i<10000&&!cpu.halted;i++){sched.event.next_cycle=cpu.cycles+100;cpu_execute();}
    if(!cpu.halted){fprintf(stderr,"Timeout PC %06x\n",cpu.registers.PC);exit(2);}
}
static bool bank_check(unsigned wram,unsigned bank,bool stack) {
    unsigned base=wram-0xc000;
    for(unsigned i=0;i<8;i++)mem_poke_byte(0xd40000+SYM_wram_bank_base_lut+i,((base>>8)+(i?i-1:0)*16-32)&255);
    mem_poke_word(SYM_wram_bank_base_for_read,base,true);mem_poke_word(SYM_wram_bank_base_for_write,base,true);mem_poke_word(SYM_wram_bank_base,base,true);mem_poke_word(SYM_wram_mirror_bank_base,base-0x2000,true);
    unsigned site=0xd40000+SYM_writeSVBK_stack_smc;
    mem_poke_byte(site,stack?0x18:0x25);mem_poke_byte(site+1,stack?(SYM_writeSVBK_stack-SYM_writeSVBK_stack_smc-2)&255:0x25);
    cpu.registers.AF=bank<<8;cpu.registers._HL=bank;cpu.registers.IY=wram+0x1fff;
    run(SYM_writeSVBKhandler,false);
    unsigned selected=bank?bank:1,expected=base+(selected-1)*0x1000;
    return mem_peek_word(SYM_wram_bank_base_for_read,true)==expected && mem_peek_word(SYM_wram_bank_base_for_write,true)==expected && mem_peek_word(SYM_wram_mirror_bank_base,true)==expected-0x2000 && (!stack||cpu.registers.IY==wram+selected*0x1000+0xfff);
}
int main(void) {
    asic_init();mem.flash.size=0x400000;mem.flash.block=calloc(mem.flash.size,1);asic_reset();control.stackLimit=0;control.protectedStart=control.protectedEnd=0;
    load("build/audio/bank-ram.bin",0xd1a881);load("build/audio/bank-archive.bin",0x100000);load("build/audio/bank-z80.bin",0xd40000);load("build/audio/bank-allocator.bin",0xd18080);
    mem_poke_word(SYM_ArcBase,0x100000-21,true);
    unsigned allocator=0x100000-21+SYM_InsertMemSafe;mem_poke_byte(allocator,0xc3);mem_poke_word(allocator+1,0xd18080,true);
    assert(!bank_check(0xd29210,4,false));assert(!bank_check(0xd29210,7,true));
    puts("Negative control: old USB layout misroutes real SVBK bank 4/7 pointers into preceding RAM.");
    unsigned offset=SYM_userMem+2*(SYM_config_size+2)+3+SYM_wram_gbc_start_offset,checks=0;
    for(unsigned low=0;low<0x10000;low+=16) {
        unsigned size=(low-offset)&0xffff;mem_poke_word(SYM_asm_prgm_size,size,true);mem_poke_word(0xd17000,0,true);mem_poke_word(0xd17003,0,true);
        run(0x100000-21+SYM_AlignGbcStateBanks,true);assert(!cpu.registers.flags.C);
        unsigned final_size=mem_peek_word(SYM_asm_prgm_size,true),wram=offset+final_size;
        assert((wram&0xffff)<0x6000);assert(final_size-size==(low<0x6000?0:0x10000-low));
        if(low>=0x6000){assert(mem_peek_word(0xd17000,true)==SYM_userMem+size);assert(mem_peek_word(0xd17003,true)==0x10000-low);}else assert(mem_peek_word(0xd17000,true)==0);
        for(unsigned bank=0;bank<8;bank++)for(unsigned stack=0;stack<2;stack++) {
            if(!bank_check(wram,bank,stack)){fprintf(stderr,"Bank mismatch low=%04x aligned=%06x bank=%u stack=%u\n",low,wram,bank,stack);return 3;}checks++;
        }
    }
    /* The existing allocation failure path must remain visible to Startup. */
    mem_poke_byte(0xd18080,0x37);mem_poke_byte(0xd18081,0xc9);unsigned size=(0x9200-offset)&0xffff;mem_poke_word(SYM_asm_prgm_size,size,true);
    run(0x100000-21+SYM_AlignGbcStateBanks,true);assert(cpu.registers.flags.C);assert(mem_peek_word(SYM_asm_prgm_size,true)==size);
    printf("PASS: native startup placement + %u SVBK/mirror/direct-stack checks across all 4096 aligned placements; allocation failure preserved.\n",checks);return 0;
}
