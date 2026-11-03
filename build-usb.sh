#!/bin/bash
set -eu
cd -- "$(dirname -- "$0")"
python3 tools/build-audio.py
mkdir -p build/usb-audio
spasm -E -A -DUSB_AUDIO -DVERSION='"USB audio prototype 2"' tiboyce.asm build/usb-audio/TBUSBDat.8xv
spasm -E -A -DUSB_AUDIO launcher.asm build/usb-audio/TIBUSB2.8xp

python3 tools/package-audio.py
