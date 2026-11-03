#!/bin/bash
set -eu
cd -- "$(dirname -- "$0")"
python3 tools/build-audio.py
mkdir -p build/usb-audio
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB004"' -DVERSION='"USB audio prototype 4"' tiboyce.asm build/usb-audio/TBUSB004.8xv
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB004"' launcher.asm build/usb-audio/TIBUSB4.8xp

python3 tools/package-audio.py
