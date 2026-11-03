#!/usr/bin/env python3
"""Package the current experimental launcher/core pair with checksum checks."""
from pathlib import Path
import zipfile
root=Path(__file__).resolve().parent.parent
out=root/'build/usb-audio'
with zipfile.ZipFile(out/'tiboyce-usb-audio-2.zip','w',zipfile.ZIP_DEFLATED) as z:
    for name in ['TIBUSB2.8xp','TBUSBDat.8xv']:
        p=out/name;b=p.read_bytes()
        assert b[:8]==b'**TI83F*' and len(b)<65535
        assert (sum(b[55:-2])&65535)==int.from_bytes(b[-2:],'little'),name
        z.write(p,name)
    z.write(root/'audio/README.md','README.md')
    z.write(root/'LICENSE.md','LICENSE.md')
print(out/'tiboyce-usb-audio-2.zip')
