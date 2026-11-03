/* ROM-free isolated eZ80 timing, using the CEmu CPU core. */
#include "emu.h"
#include "asic.h"
#include "cpu.h"
#include "mem.h"
#include "control.h"
#include "flash.h"
#include "symbols.h"

#define CHECK_CHUNKS 120

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
void gui_console_clear(void){}
void gui_console_printf(const char *f,...){(void)f;}
void gui_console_err_printf(const char *f,...){(void)f;}
asic_rev_t gui_handle_reset(const boot_ver_t*v,asic_rev_t l,asic_rev_t d,emu_device_t dev,bool*python){(void)v;(void)l;(void)d;(void)dev;*python=false;return ASIC_REV_I;}
static unsigned run(unsigned pc){cpu_clear_signals();cpu.halted=false;cpu.ADL=cpu.MADL=cpu.L=cpu.IL=true;cpu.IEF1=cpu.IEF2=false;cpu.registers.SPL=0xd17f00;mem_poke_word(0xd17f00,0xd18020,true);cpu_flush(pc,true);unsigned start=cpu.cycles;for(unsigned i=0;i<1000000;i++){sched.event.next_cycle=cpu.cycles+1000;cpu_execute();if(cpu.halted)return cpu.cycles-start;}fprintf(stderr,"Timeout PC=%06x IX=%06x SP=%06x counter=%06x\n",cpu.registers.PC,cpu.registers.IX,cpu.registers.SPL,mem_peek_word(cpu.registers.IX-19,true));exit(1);}
int main(int argc,char**argv){if(argc!=3&&argc!=4)return 1;asic_init();mem.flash.size=0x400000;mem.flash.block=calloc(mem.flash.size,1);asic_reset();set_cpu_clock(48000000);control.stackLimit=0;control.protectedStart=control.protectedEnd=0;FILE*f=fopen(argv[1],"rb");fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);fread(phys_mem_ptr(0xd1e881,n),1,n,f);fclose(f);mem_poke_byte(0xd18020,0x76);if(argc==4)mem_poke_word(SYM_tba_bench_setting,strtoul(argv[3],0,0),true);run(SYM_tba_bench_init);f=fopen(argv[2],"wb");unsigned long long sum=0;unsigned max=0;for(unsigned i=0;i<CHECK_CHUNKS;i++){unsigned c=run(SYM_tba_bench_frame);sum+=c;if(c>max)max=c;fwrite(phys_mem_ptr(mem_peek_word(0xd17000,true),536),1,536,f);}fclose(f);if(mem_peek_word(SYM_tba_poll_calls,true)<2000){fprintf(stderr,"Mixer did not service USB during synthesis\n");return 2;}printf("Native four-channel APU: mean %.3f ms / max %.3f ms per 268 samples at 48MHz\n",sum/(CHECK_CHUNKS*48000.),max/48000.);return 0;}