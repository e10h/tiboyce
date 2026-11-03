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
run(['cc','-O2','-Wall','-Wextra','-DTBA_HOST_TEST','-I'+str(root/'audio/test-support'),root/'audio/apu.c',root/'audio/usb.c',root/'audio/test-usb-clock.c','-o',out/'test-usb-clock'])
run([out/'test-usb-clock'])
if args.cemu:
    run(['python3',root/'tools/build-audio.py','--bench'])
    core=args.cemu.resolve()/'core'
    run(['make','-C',core,'-j4','CC=clang','CFLAGS=-O3 -std=gnu11 -fPIC'],stdout=subprocess.DEVNULL)
    run(['clang','-O2','-I'+str(core),'-I'+str(out),root/'tools/apu-cycles.c',core/'libcemucore.a','-o',out/'cycles','-framework','IOKit','-framework','CoreFoundation'])
    run(['cc','-O2',root/'audio/apu.c',root/'audio/bench.c','-o',out/'reference'])
    results=[]
    for rate in [4000,8000,16000]:
      for setting in [0x00,0x08,0x35,0xff]:
        result=run([out/'cycles',out/'module.bin',out/f'native-{rate}-{setting:02x}.pcm',hex(setting),str(rate)],capture_output=True,text=True).stdout
        ref=run([out/'reference',hex(setting),str(rate)],capture_output=True).stdout
        assert ref==(out/f'native-{rate}-{setting:02x}.pcm').read_bytes(),f'Native mixer differs for {rate} Hz NR43={setting:02x}'
        line=f'{rate} Hz NR43={setting:02x}: '+result
        print(line,end='');results.append(line)
    print('All 12 rate/noise cases: 120 four-channel frames each match the C reference.')
    (out/'result.txt').write_text(''.join(results)+'All noise-setting PCM parity checks passed.\n')
    # Restore the normal module: benchmark functions are not included in releases.
    run(['python3',root/'tools/build-audio.py'])
