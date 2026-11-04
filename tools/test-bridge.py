#!/usr/bin/env python3
"""Check both native/mixed-mode USB poll bridges on the ROM-free CEmu CPU."""
import argparse,subprocess,re
from pathlib import Path
root=Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser();p.add_argument('--cemu',type=Path,required=True);p.add_argument('--spasm',default='spasm');p.add_argument('--diagnostic',action='store_true');a=p.parse_args()
out=root/'build/audio'
def run(cmd,**kw):return subprocess.run([str(x) for x in cmd],check=True,**kw)
run([a.spasm,'-E','-A','-L','-DUSB_AUDIO',*(['-DUSB_DIAGNOSTIC'] if a.diagnostic else []),root/'tiboyce.asm',out/'BRGCORE.8xv'],cwd=root,stdout=subprocess.DEVNULL)
lab=(out/'BRGCORE.lab').read_text()
def label(name):return int(re.search(r'^'+name+r' = \$([0-9A-Fa-f]+)',lab,re.M)[1],16)
(out/'bridge.h').write_text('\n'.join(f'#define {n} 0x{label(n):x}' for n in ['AudioPoll','AudioPollZ80','AudioStart','AudioFrame','AudioStop','tba_poll','tba_call_stack','tba_disable_usb']+(['AudioDiagnosticConfigure','audio_diagnostic_mode'] if a.diagnostic else []))+'\n')
(out/'bridge-stub.asm').write_text(f' .assume adl=0\n .org $8000\n call.il ${label("AudioPollZ80"):x}\n halt\n')
run([a.spasm,'-E','-A',out/'bridge-stub.asm',out/'bridge-stub.bin'],stdout=subprocess.DEVNULL)
b=(out/'BRGCORE.8xv').read_bytes();i=b.index(b'TIBoyEXE\x01')+9;n=int.from_bytes(b[i:i+2],'little');(out/'emulator-ram.bin').write_bytes(b[i+2:i+2+n])
(out/'bridge-deep.asm').write_text(' .assume adl=1\n .org $D18080\n ld hl,-1536\n add hl,sp\n ld sp,hl\n ld (hl),$CC\n push hl\n pop de\n inc de\n ld bc,1535\n ldir\n ld hl,1536\n add hl,sp\n ld sp,hl\n ret\n')
run([a.spasm,'-E','-A',out/'bridge-deep.asm',out/'bridge-deep.bin'],stdout=subprocess.DEVNULL)
core=a.cemu.resolve()/'core'
run(['clang','-O2','-I'+str(core),'-I'+str(out),root/'tools/bridge-cycles.c',core/'libcemucore.a','-o',out/'bridge-test','-framework','IOKit','-framework','CoreFoundation'])
run([out/'bridge-test',out/'emulator-ram.bin',out/'bridge-stub.bin',out/'bridge-deep.bin'])
