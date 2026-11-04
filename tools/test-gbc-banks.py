#!/usr/bin/env python3
"""ROM-free native regression for USB-expanded GBC state/bank placement."""
from pathlib import Path
import argparse,subprocess,re
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--cemu',type=Path,required=True);p.add_argument('--spasm',default='spasm');a=p.parse_args();out=root/'build/audio'
def run(args,**kw):return subprocess.run(list(map(str,args)),check=True,cwd=root,**kw)
run([a.spasm,'-E','-A','-L','-DUSB_AUDIO','tiboyce.asm',out/'BANKCORE.8xv'],stdout=subprocess.DEVNULL)
lab=(out/'BANKCORE.lab').read_text();labels={n:int(v,16) for n,v in re.findall(r'^(\w+) = \$([0-9A-Fa-f]+)',lab,re.M)}
names=['ArcBase','AlignGbcStateBanks','InsertMemSafe','asm_prgm_size','config_size','wram_gbc_start_offset','userMem','z80code','z80codesize','writeSVBKhandler','writeSVBK_stack','writeSVBK_stack_smc','wram_bank_base_for_read','wram_bank_base_for_write','wram_bank_base','wram_mirror_bank_base','wram_bank_base_lut','program_size']
(out/'banks.h').write_text('\n'.join(f'#define SYM_{name} 0x{labels[name]:x}u' for name in names)+'\n')
b=(out/'BANKCORE.8xv').read_bytes();start=b.index(b'TIBoyEXE\x01');(out/'bank-archive.bin').write_bytes(b[start:-2]);(out/'bank-ram.bin').write_bytes(b[start+11:start+11+labels['program_size']]);z=start+labels['z80code']-21;(out/'bank-z80.bin').write_bytes(b[z:z+labels['z80codesize']])
(out/'bank-allocator.asm').write_text(f' .assume adl=1\n .org $D18080\n ld ($D17000),hl\n ld ($D17003),de\n ld hl,(${labels["asm_prgm_size"]:x})\n add hl,de\n ld (${labels["asm_prgm_size"]:x}),hl\n or a\n ret\n')
run([a.spasm,'-E','-A',out/'bank-allocator.asm',out/'bank-allocator.bin'],stdout=subprocess.DEVNULL)
core=a.cemu.resolve()/'core'
run(['clang','-O2','-I'+str(core),'-I'+str(out),root/'tools/gbc-bank-cycles.c',core/'libcemucore.a','-o',out/'bank-test','-framework','IOKit','-framework','CoreFoundation'])
run([out/'bank-test'])
