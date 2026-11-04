# TI-Boy CE USB audio — experimental prototype 10

This fork generates 16 kHz mono PCM on the calculator and sends it to Doomvision's
browser USB receiver. It does not stream key events or play a prerecorded song.

## Try it

1. Transfer **TBAUD10.8xg**, which sends both TIBUSB10 and TBUSB010. Existing game
   AppVars and archived LibLoad / USBDRVCE are reused.
2. Exit the old emulator, run **TIBUSB10**, then load your game.
3. Quit TI Connect CE and connect **TI-Boy CE PCM10** on Doomvision `/usb` in
   Chrome. Fast 4 kHz is the default. PCM7 enables live 8/16 kHz choices; PCM6
   remains accepted in Fast-only mode. Higher rates cost more game CPU time.
4. The browser discards startup backlog and briefly rebuffers on a rate change.

## GBC state placement (PCM10)

Super Mario Bros. Deluxe crashed entering level 1 in PCM7/8, including with
USB unplugged. The stock emulator works. Diagnostic CALL mode (C bridges
active, no usb_Init) crashed; SAFE mode (C calls bypassed) eventually reported
a runtime error. The 2 KiB stack isolation in PCM8 did not resolve this.

The USB-expanded program and loaded driver place GBC WRAM at a different RAM
address. The native SVBK handler updates only pointer H bytes, including the
direct WRAM stack pointer's IYH, and does not carry into the upper address byte.
For example, WRAM at D29210 misroutes banks 4 and 7 into earlier RAM containing
native emulator/audio memory. PCM10 aligns startup allocation so predicted
GBC WRAM starts below offset 6000 of a 64 KiB region. Unsafe placements are
padded to the next 64 KiB boundary, keeping normal, mirrored and direct-stack
bank mappings valid. The original 16-byte VRAM alignment is preserved.

The ROM-free native regression executes the real startup alignment helper and
SVBK machine code. It reproduces the old bad mapping, then checks all eight
bank values in normal/direct-stack modes at all 4096 aligned placements:
65,536 bank checks pass. Allocation failure still follows the existing error
path. Hardware verification of the Deluxe level transition is pending.
Padding uses additional runtime RAM, not additional track storage; if a game
cannot fit, startup reports insufficient RAM rather than allowing bank aliases.

## Stack isolation (PCM8)

The user reported Super Mario Bros. Deluxe clearing calculator RAM on entry
into level 1 in TIBUSB7, while original TIBOYCE works. PCM8 gives all bridged
C/USB calls a dedicated 2 KiB stack in the allocated audio module. The old
bridge borrowed the emulator/JIT stack. A ROM-free negative-control test
shows a 1536-byte C call overwriting the emulator stack guard with that bridge;
the isolated bridge preserves the guard in both ADL and Z80 CALL.IL modes,
while restoring both register banks, I, IX/IY and both stack pointers.
This demonstrates a stack-safety defect; GBC bank placement above was a
separate confirmed mapping defect.
The TX ring has twenty slots to keep the core and transport group within
TI variable size limits after reserving the dedicated stack.

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
the selected rate and clears old data at changes. A direct PCM6 Fast-mode capture measured approximately 4,004 PCM samples/s
with zero missing chunks and a 47.2 ms maximum packet gap. Higher-rate vendor
requests were acknowledged but did not change the physical output from 4 kHz;
the browser disables those choices for PCM6. The user confirmed perfect
gameplay in Fast mode. PCM7 fixes the control handler by taking the validated
rate directly from the accepted setup request, rather than relying on the
legacy driver's zero-length callback data layout. Both callback layouts are
covered by the host regression test. Physical PCM7 switching was verified on 2026-11-03: 8 kHz measured
7,998 samples/s (maximum packet gap 31.5 ms), 16 kHz measured 15,959 samples/s
(27.6 ms), and restoring 4 kHz measured 3,990 samples/s (45.7 ms). All three
captures had zero missing chunks and the expected rate in the PCM headers.

Vendor control OUT (device recipient), request 0x51, wValue 4000/8000/16000,
wIndex 0, zero payload selects the rate. Once scheduling the control acknowledgement succeeds, the accepted rate is
recorded and applies at a frame boundary, preserving oscillator
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
python3 tools/test-gbc-banks.py --cemu /path/to/CEmu
```

The CEmu check runs the real eZ80 mixer without a calculator ROM and compares its
output to the portable model. The script restores the production module after
benchmarking; rerun `build-usb.sh` to assemble the release pair.

Source: https://github.com/e10h/tiboyce/tree/codex/usb-audio
The fork and this integration are distributed under upstream's GPLv3 license.
