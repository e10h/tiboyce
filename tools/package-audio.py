#!/usr/bin/env python3
"""Package the current experimental launcher/core pair with checksum checks."""
from pathlib import Path
import zipfile
root=Path(__file__).resolve().parent.parent
out=root/'build/usb-audio'
with zipfile.ZipFile(out/'tiboyce-usb-audio-8.zip','w',zipfile.ZIP_DEFLATED) as z:
    for name in ['TIBUSB8.8xp','TBUSB008.8xv']:
        p=out/name;b=p.read_bytes()
        assert b[:8]==b'**TI83F*' and len(b)<65535
        assert (sum(b[55:-2])&65535)==int.from_bytes(b[-2:],'little'),name
        if name.endswith('.8xp'):
            assert b[60:68].rstrip(b'\0')==b'TIBUSB8', 'Launcher variable name mismatch'
            assert b'TBUSB008' in b and b'TBUSBDat' not in b, 'Launcher must target its versioned core'
        else:
            assert 'TI-Boy CE PCM8'.encode('utf-16le') in b, 'Wrong firmware identity in core'
            assert b[60:68]==b'TBUSB008' and b'TBUSB008' in b[72:], 'Core variable / SelfName mismatch'
        z.write(p,name)
    z.write(root/'audio/README.md','README.md')
    z.write(root/'LICENSE.md','LICENSE.md')
print(out/'tiboyce-usb-audio-8.zip')

# A transport group contains ordinary variable records, like clibs.8xg. This
# differs from a type-17 on-calculator Group, which requires manual ungrouping.
group=out/'TBAUD8.8xg'
files=[(out/name).read_bytes() for name in ['TIBUSB8.8xp','TBUSB008.8xv']]
header=bytearray(files[0][:53]);header[11:53]=b'TI-Boy CE USB audio 8'.ljust(42,b'\0')
payload=b''.join(b[55:-2] for b in files)
assert len(payload)<65536
group.write_bytes(header+len(payload).to_bytes(2,'little')+payload+(sum(payload)&65535).to_bytes(2,'little'))
b=group.read_bytes();assert (sum(b[55:-2])&65535)==int.from_bytes(b[-2:],'little')
offset=55;names=[]
while offset<len(b)-2:
    h=int.from_bytes(b[offset:offset+2],'little');n=int.from_bytes(b[offset+2:offset+4],'little')
    names.append(b[offset+5:offset+13].rstrip(b'\0'));offset+=h+4+n
assert names==[b'TIBUSB8',b'TBUSB008'] and offset==len(b)-2
print(f'{group}: both uniquely named variables verified')
