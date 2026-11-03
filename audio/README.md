# TI-Boy CE USB audio — experimental prototype 1

This fork generates 16 kHz mono PCM on the calculator and sends it to Doomvision's
browser USB receiver. It does not stream key events or play a prerecorded song.

## Try it

1. Transfer **both** `TIBOYUSB.8xp` and `TBUSBDat.8xv` from `build/usb-audio/`.
   These have separate names from the stock launcher/core. Existing converted
   game AppVars are reused. LibLoad and USBDRVCE must already be installed and
   archived (the same libraries used by DVUSB6).
2. Run **TIBOYUSB** and load your game. The emulator initializes USB when the
   game starts; reconnect the USB cable after starting the game if needed.
3. Close TI Connect CE and any native Doomvision receiver. Open the updated
   Doomvision `/usb` page in Chrome on Mac, click Connect calculator, choose
   **TI-Boy CE PCM1**, then Connect. The mode should read **Game Boy**.
4. Exiting emulation or entering a menu cleans up USB before TI-OS file calls.
   After resuming, reconnect from the browser if it disconnected.

This is not yet verified with a physical calculator running a game. Do not
interpret the isolated mixer benchmark as end-to-end gameplay validation.

## Current accuracy and performance limits

All four channels are present: two pulse channels, wave RAM playback and LFSR
noise, with channel routing, DAC enable, envelopes, lengths and a first sweep
implementation. Mixing is mono, with DC removal and headroom.

Control changes are sampled at VBlank. Trigger bits in TI-Boy's private raw
register mirror are consumed there; CPU-visible register masks are separate.
Multiple triggers inside one frame collapse into one. Envelopes and sweep advance
on the 512 Hz frame-sequencer clock, with their audible controls applied in video
frame batches. This is **not cycle-accurate APU emulation**: subframe writes,
hardware quirks, sweep edge cases, wave-RAM access quirks and alias suppression
need further work. A game that cannot sustain normal emulation speed can starve
the audio queue; automatic frameskip may help.

The initial all-C sample loop took about 120 ms per 268 samples at 48 MHz.
The native mixer takes approximately 7.54 ms mean / 9.43 ms maximum in the
four-channel fixture. This excludes USB and the rest of emulation, and does not
bound every possible noise-frequency setting. 120 native four-channel frames
match the portable reference byte for byte.

## Integration

- `apu.c/h`: portable register/control model and reference mixer.
- `mix.S`: eZ80 PCM kernel; no TI ROM is needed for the isolated test.
- `usb.c`: queued bulk endpoint 0x81, existing DVU1 framing, flag 4 = Game Boy.
- `imports.S`: LibLoad imports for USBDRVCE. Resolution happens before config/ROM
  allocation. Libraries must be archived; the resolver never archives LibLoad.
- `bridge.asm`: preserves both CPU register banks, IX/IY and the I event counter
  around ADL C calls, and retains the emulator's disabled-interrupt discipline.
- Module starts at 0xD1E881, after a guarded padding region in the user-memory
  image. Module BSS is embedded and allocated with that image, not in TI-Boy's
  rendering/ROM-cache SafeRAM. USBDRVCE uses its dedicated usbArea and OS heap.
- Init occurs after emulator hardware configuration, frame service at VBlank,
  cleanup before state/file operations and hardware restoration.

## Build / verify

Install CEdev and the SPASM-ng revision required by upstream. Set `CEDEV` if it
is not `~/CEdev`, and put `spasm` on PATH. Run:

```
bash build-usb.sh
python3 tools/test-audio.py
python3 tools/test-audio.py --cemu /path/to/CEmu
```

The CEmu check runs the real eZ80 mixer without a calculator ROM and compares its
output to the portable model. The script restores the production module after
benchmarking; rerun `build-usb.sh` to assemble the release pair.

Source: https://github.com/e10h/tiboyce/tree/codex/usb-audio
The fork and this integration are distributed under upstream's GPLv3 license.
