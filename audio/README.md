# TI-Boy CE USB audio — experimental prototype 5

This fork generates 16 kHz mono PCM on the calculator and sends it to Doomvision's
browser USB receiver. It does not stream key events or play a prerecorded song.

## Try it

1. Transfer **TBAUD5.8xg**. This transport group sends both TIBUSB5 and TBUSB005
   in one operation; it is not an on-calculator Group requiring ungrouping.
   Alternatively transfer both files from the zip. Existing game AppVars are
   reused; the installed LibLoad and USBDRVCE libraries must be archived.
2. Exit the old emulator. Run **TIBUSB5**, then load your game.
3. Quit TI Connect CE, reconnect the cable, and select **TI-Boy CE PCM5** on
   Doomvision's updated `/usb` page in Chrome. For a raw timing capture, leave
   the browser disconnected so the capture tool can claim the interface.

## Verified failure and changes

A direct live capture of **PCM4** measured ~38 chunks/s (~4,846 samples/s), with
roughly 87 missing sequence numbers/s. The 38+87 total matches the intended
125 chunks/s: audio was being generated but discarded before delivery. The
browser then necessarily underruns. This is separate from earlier stale-core
packaging; the PCM4 identity was read from the device itself.

This version services USB **inside the assembly sample loop** every 16 samples,
not only between game events. It also polls during scanline rendering and grows
the queued-transfer ring from 8 to 24 slots. Heavy noise generation uses an exact
8-clock LFSR advance. Four sample tables use aligned cells to reduce lookup cost;
DC rejection runs on the Mac, not in the calculator mixer. Game Boy playback uses
a 128 ms priming buffer and a 320 ms queue cap; the piano receiver keeps its
existing 64 ms / 200 ms behavior.

The native checks compare 120 frames for each of eight noise settings against
the portable reference and require >2,000 USB service calls during synthesis.
Both ADL and Z80-mode bridge checks preserve both register banks, I, IX/IY and
stack modes. Continuous physical PCM5 playback remains to be measured.

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

The initial all-C sample loop took about 120 ms per 268 samples at 48 MHz.
Expanded tests now include sustained noise at extreme frequencies and both LFSR
widths. The optimized mixer ranges from about 6.4 to 11.5 ms mean per 268 samples
in those fixtures, with a 13.3 ms worst observed frame. This includes the USB
service call path with an inactive test driver, but excludes actual hardware USB
work and the rest of emulation. It is not a claim of end-to-end gameplay speed.

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
