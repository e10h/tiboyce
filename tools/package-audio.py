#!/usr/bin/env python3
"""Package the current experimental launcher/core pair with checksum checks."""
from pathlib import Path
import zipfile
root=Path(__file__).resolve().parent.parent
out=root/'build/usb-audio'
with zipfile.ZipFile(out/'tiboyce-usb-audio-4.zip','w',zipfile.ZIP_DEFLATED) as z:
    for name in ['TIBUSB4.8xp','TBUSB004.8xv']:
        p=out/name;b=p.read_bytes()
        assert b[:8]==b'**TI83F*' and len(b)<65535
        assert (sum(b[55:-2])&65535)==int.from_bytes(b[-2:],'little'),name
        if name.endswith('.8xp'):
            assert b[60:68].rstrip(b'\0')==b'TIBUSB4', 'Launcher variable name mismatch'
            assert b'TBUSB004' in b and b'TBUSBDat' not in b, 'Launcher must target its versioned core'
        else:
            assert 'TI-Boy CE PCM4'.encode('utf-16le') in b, 'Wrong firmware identity in core'
            assert b[60:68]==b'TBUSB004' and b'TBUSB004' in b[72:], 'Core variable / SelfName mismatch'
        z.write(p,name)
    z.write(root/'audio/README.md','README.md')
    z.write(root/'LICENSE.md','LICENSE.md')
print(out/'tiboyce-usb-audio-4.zip')
