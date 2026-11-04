#!/bin/bash
set -eu
cd -- "$(dirname -- "$0")"
python3 tools/build-audio.py
mkdir -p build/usb-audio
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB008"' -DVERSION='"USB audio prototype 8"' tiboyce.asm build/usb-audio/TBUSB008.8xv
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB008"' launcher.asm build/usb-audio/TIBUSB8.8xp

python3 tools/package-audio.py
