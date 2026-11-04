#!/bin/bash
set -eu
cd -- "$(dirname -- "$0")"
python3 tools/build-audio.py
mkdir -p build/usb-audio
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB010"' -DVERSION='"USB audio prototype 10"' tiboyce.asm build/usb-audio/TBUSB010.8xv
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB010"' launcher.asm build/usb-audio/TIBUSB10.8xp

python3 tools/package-audio.py
