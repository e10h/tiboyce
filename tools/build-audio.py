#!/usr/bin/env python3
"""Build the fixed-address C audio extension and export symbols to SPASM."""
from pathlib import Path
import os, subprocess, sys
root=Path(__file__).resolve().parent.parent
ce=Path(os.environ.get('CEDEV',Path.home()/'CEdev'))
out=root/'build/audio';out.mkdir(parents=True,exist_ok=True)
def run(args):subprocess.run([str(a) for a in args],check=True,cwd=root)
cc=ce/'bin/ez80-clang';bin=ce/'binutils/bin';objects=[]
bench='--bench' in sys.argv
for name in ['apu','usb']+(['bench'] if bench else []):
    asm=out/(name+'.s');obj=out/(name+'.o')
    run([cc,'-S','-nostdinc','-isystem',ce/'include','-D__TICE__=1',*(['-DTBA_BENCH=1'] if bench else []),'-fno-autolink','-fno-addrsig','-mllvm','-z80-gas-style','-Os','-fno-omit-frame-pointer','-ffunction-sections','-fdata-sections',root/'audio'/(name+'.c'),'-o',asm])
    run([bin/'z80-none-elf-as','-march=ez80+full',asm,'-o',obj]);objects.append(obj)
run([bin/'z80-none-elf-as','-march=ez80+full',root/'audio/mix.S','-o',out/'mix.o'])
objects.append(out/'mix.o')
run([bin/'z80-none-elf-as','-march=ez80+full',root/'audio/imports.S','-o',out/'imports.o'])
run([bin/'z80-none-elf-ld','--entry=_tba_resolve','--gc-sections','--no-warn-rwx-segments','--undefined=_tba_start','--undefined=_tba_stop','--undefined=_tba_frame','--undefined=_tba_poll',*(['--undefined=_tba_bench_init','--undefined=_tba_bench_frame'] if bench else []),'-T',root/'audio/module.ld',*objects,out/'imports.o','--start-group',ce/'lib/libc/libc.a',ce/'lib/crt/libcrt.a','--end-group','-o',out/'module.elf'])
run([bin/'z80-none-elf-objcopy','-O','binary',out/'module.elf',out/'module.bin'])
symbols=subprocess.check_output([bin/'z80-none-elf-nm',out/'module.elf'],text=True)
exports={parts[2]:int(parts[0],16) for line in symbols.splitlines() if len(parts:=line.split())==3}
end=exports['_module_end'];base=0xd1e881
# BSS is explicitly embedded in TI-Boy's allocated user-memory image.
p=out/'module.bin';p.write_bytes(p.read_bytes().ljust(end-base,b'\0'))
(out/'symbols.inc').write_text('\n'.join(f'{key[1:]} = ${exports[key]:06X}' for key in ['_tba_resolve','_tba_start','_tba_stop','_tba_frame','_tba_regs_ptr','_tba_poll'])+'\n')
(out/'symbols.h').write_text('\n'.join(f'#define SYM{key} 0x{value:x}u' for key,value in exports.items() if key.startswith('_tba_') and '.' not in key)+'\n')
print(f'Audio module: {end-base} bytes (including state and USB buffers)')
