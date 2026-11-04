#!/usr/bin/env python3
"""Build matched-layout SAFE/CALL diagnostic launchers; never includes a ROM."""
from pathlib import Path
import subprocess,os,zipfile
root=Path(__file__).resolve().parents[1];out=root/'build/usb-diagnostic9';out.mkdir(parents=True,exist_ok=True)
spasm=os.environ.get('SPASM','spasm')
def run(args):subprocess.run(list(map(str,args)),check=True,cwd=root,stdout=subprocess.DEVNULL)
run(['python3','tools/build-audio.py'])
run([spasm,'-E','-A','-L','-DUSB_AUDIO','-DUSB_DIAGNOSTIC','-DUSB_CORE_NAME="TBUSBD09"','-DVERSION="USB diagnostic 9"','tiboyce.asm',out/'TBUSBD09.8xv'])
for name,mode in [('T9SAFE',2),('T9CALL',1)]:
    run([spasm,'-E','-A','-DUSB_AUDIO','-DUSB_CORE_NAME="TBUSBD09"',f'-DUSB_DIAGNOSTIC_MODE={mode}','launcher.asm',out/(name+'.8xp')])
    b=(out/(name+'.8xp')).read_bytes()
    assert bytes([0xe3,0x3e,mode,0xc9]) in b,'Launcher must pass its mode through LDIR to Startup'
paths=[out/'T9SAFE.8xp',out/'T9CALL.8xp',out/'TBUSBD09.8xv']
files=[p.read_bytes() for p in paths]
for p,b in zip(paths,files):
    assert b[:8]==b'**TI83F*' and sum(b[55:-2])&65535==int.from_bytes(b[-2:],'little'),p
payload=b''.join(b[55:-2] for b in files);assert len(payload)<65536
header=bytearray(files[0][:53]);header[11:53]=b'TI-Boy USB diagnostic SAFE/CALL'.ljust(42,b'\0')
group=out/'T9DIAG.8xg';group.write_bytes(header+len(payload).to_bytes(2,'little')+payload+(sum(payload)&65535).to_bytes(2,'little'))
(out/'README.txt').write_text('''TI-Boy USB diagnostic 9 - not a crash fix

Transfer T9DIAG.8xg, which installs T9SAFE, T9CALL and TBUSBD09.
Keep the existing game AppVars and archived LibLoad / USBDRVCE libraries.
Unplug the USB cable before each test. Restart the game fresh in each mode.

1. Run T9SAFE, load Super Mario Bros. Deluxe and enter level 1.
2. Run T9CALL and try the same level.

T9SAFE bypasses C calls during emulation and does not initialize USB.
T9CALL keeps the C bridge/APU setup calls but suppresses usb_Init, USB
servicing and PCM generation. Neither mode streams audio.
Both launchers use the exact same allocated core and loaded driver image,
so a differing result isolates behavior without changing the memory layout.
The driver loader still runs before either mode starts, and the existing
scheduler/scanline wrapper calls remain in the SAFE path with no-op targets.

Report whether both work, only SAFE works, only CALL works, or both crash.
The original TIBOYCE is the known working control. TIBUSB8 crashes even
unplugged, so active PCM synthesis has already been excluded.

ROM-free checks verify mode selection/no-op return modes, register/stack
preservation under 1536-byte C calls and no USB initialization in CALL mode.
No copyrighted game or calculator ROM is included.
''')
with zipfile.ZipFile(out/'tiboy-usb-diagnostic9.zip','w',zipfile.ZIP_DEFLATED) as z:
    z.write(group,group.name);z.write(out/'README.txt','README.txt')
print(group)
print(f'{len(payload)}-byte transport payload; both launchers and common core checksummed')
