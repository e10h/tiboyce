#!/usr/bin/env python3
"""Host checks plus ROM-free native mixer parity/timing (optional --cemu PATH)."""
from pathlib import Path
import argparse,subprocess,os
root=Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser();p.add_argument('--cemu',type=Path);args=p.parse_args()
out=root/'build/audio';out.mkdir(parents=True,exist_ok=True)
def run(cmd,**kwargs):return subprocess.run([str(x) for x in cmd],check=True,**kwargs)
run(['cc','-O2','-Wall','-Wextra',root/'audio/apu.c',root/'audio/test-apu.c','-o',out/'test-apu'])
run([out/'test-apu'])
if args.cemu:
    run(['python3',root/'tools/build-audio.py','--bench'])
    core=args.cemu.resolve()/'core'
    run(['make','-C',core,'-j4','CC=clang','CFLAGS=-O3 -std=gnu11 -fPIC'],stdout=subprocess.DEVNULL)
    run(['clang','-O2','-I'+str(core),'-I'+str(out),root/'tools/apu-cycles.c',core/'libcemucore.a','-o',out/'cycles','-framework','IOKit','-framework','CoreFoundation'])
    result=run([out/'cycles',out/'module.bin',out/'native.pcm'],capture_output=True,text=True).stdout
    run(['cc','-O2',root/'audio/apu.c',root/'audio/bench.c','-o',out/'reference'])
    ref=run([out/'reference'],capture_output=True).stdout
    assert ref==(out/'native.pcm').read_bytes(),'Native mixer differs from reference'
    print(result,end='');print('120 four-channel frames: native PCM is byte-identical to the C reference.')
    (out/'result.txt').write_text(result+'120 four-channel frames: PCM parity passed.\n')
    # Restore the normal module: benchmark functions are not included in releases.
    run(['python3',root/'tools/build-audio.py'])
