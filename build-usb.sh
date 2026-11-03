#!/bin/bash
set -eu
cd -- "$(dirname -- "$0")"
python3 tools/build-audio.py
mkdir -p build/usb-audio
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB007"' -DVERSION='"USB audio prototype 7"' tiboyce.asm build/usb-audio/TBUSB007.8xv
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB007"' launcher.asm build/usb-audio/TIBUSB7.8xp

python3 tools/package-audio.py
