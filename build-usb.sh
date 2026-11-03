#!/bin/bash
set -eu
cd -- "$(dirname -- "$0")"
python3 tools/build-audio.py
mkdir -p build/usb-audio
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB005"' -DVERSION='"USB audio prototype 5"' tiboyce.asm build/usb-audio/TBUSB005.8xv
spasm -E -A -DUSB_AUDIO -DUSB_CORE_NAME='"TBUSB005"' launcher.asm build/usb-audio/TIBUSB5.8xp

python3 tools/package-audio.py
