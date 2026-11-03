# TI-Boy CE USB audio — experimental prototype 6

This fork generates 16 kHz mono PCM on the calculator and sends it to Doomvision's
browser USB receiver. It does not stream key events or play a prerecorded song.

## Try it

1. Transfer **TBAUD6.8xg**, which sends both TIBUSB6 and TBUSB006. Existing game
   AppVars and archived LibLoad / USBDRVCE are reused.
2. Exit the old emulator, run **TIBUSB6**, then load your game.
3. Quit TI Connect CE and connect **TI-Boy CE PCM6** on the updated Doomvision
   `/usb` page. The default is **Fast · 4 kHz**. The website can change between
   4, 8 and 16 kHz while the game runs; lower rates leave more CPU for gameplay.
   All four sound channels and PCM generation remain on the calculator.
4. The browser flushes startup backlog before playback and resets the queue
   during a rate change. Expect a brief rebuffer when changing quality.

## Verification and CPU budget

PCM5 was physically verified at ~125 chunks/s and zero missing chunks; the user
confirmed continuous audio but game speed was below playable. This version
reduces the default rate to 4 kHz and provides live quality selection. Native
benchmarks cover 12 rate/noise combinations, each with 120 full four-channel
frames matching the reference. At 4 kHz those cases average 2.5–3.6 ms per game
frame, versus 6.8–9.5 ms at 16 kHz; real USB work and game emulation are additional.
Pitch checks pass at all three rates. The 32/128-clock noise jumps are exact GF(2)
transforms, checked against the reference, rather than approximated noise.

Fast mode delivers 4,000 samples/s = 31.25 packets/s. Balanced mode is 8,000
samples/s = 62.5 packets/s. High mode is 16,000 samples/s = 125 packets/s.
Each packet carries its actual source rate. The browser only plays packets at
the selected rate and clears old data at changes. Physical playable-speed and
rate-control verification for PCM6 remains pending.

Vendor control OUT (device recipient), request 0x51, wValue 4000/8000/16000,
wIndex 0, zero payload selects the rate. The control callback acknowledges the
request; the pending change applies at a frame boundary, preserving oscillator
phase and envelope progress. The sender stops spending CPU on discarded audio
when its transfer ring is full because the host is not reading.

## Current accuracy and performance limits

All four channels are present: two pulse channels, wave RAM playback and LFSR
noise, with channel routing, DAC enable, envelopes, lengths and a first sweep
implementation. Mixing is mono, with headroom and DC rejection in the browser output.

Control changes are sampled at VBlank. Trigger bits in TI-Boy's private raw
register mirror are consumed there; CPU-visible register masks are separate.
Multiple triggers inside one frame collapse into one. Envelopes and sweep advance
on the 512 Hz frame-sequencer clock, with their audible controls applied in video
frame batches. This is **not cycle-accurate APU emulation**: subframe writes,
hardware quirks, sweep edge cases, wave-RAM access quirks and alias suppression
need further work. A game that cannot sustain normal emulation speed can starve
the audio queue; automatic frameskip may help.

The initial all-C sample loop took ~120 ms per 268 samples at 48 MHz. The native
sample loop services USB once per millisecond of synthesized PCM. The tests
require >2,000 service calls over each 120-frame fixture. No calculator ROM is
needed for these isolated tests; the default non-USB build is unchanged.

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
  with sample generation paced by the USB frame counter rather than game FPS,
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
