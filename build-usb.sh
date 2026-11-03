#!/bin/bash
set -eu
cd -- "$(dirname -- "$0")"
python3 tools/build-audio.py
mkdir -p build/usb-audio
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB006"' -DVERSION='"USB audio prototype 6"' tiboyce.asm build/usb-audio/TBUSB006.8xv
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB006"' launcher.asm build/usb-audio/TIBUSB6.8xp

python3 tools/package-audio.py
