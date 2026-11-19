<!-- SPDX-License-Identifier: CC0-1.0 -->
# Pictures, part B: control, utility and time kinds

What the app sends in PICTURE (cmd 20) and LIVE (cmd 21) for LFO, envelope, sequencer, arp, chord, MIDI, macro,
audio in, output, bridge, delay and reverb, so the FM-1 can redraw Steckfeld's display for each
(`App/UI/Displays/*.swift`, called from `ToolScreen.picture(_:)`). The FM-1 draws them in
`firmware/apps/fernbedienung/fb_pics_b.h`.

Conventions:

- `v[i]` is a v14 (0..16383). **frac** means a 0..1 value sent as `round(x × 16383)`, clamped.
- `arr` is the byte array (7-bit bytes). **str** is ASCII 0x20-0x7E ended by 0x00; the FM-1 draws it as sent, so
  send upper case where Steckfeld shows upper case.
- `live[i]` comes in LIVE, up to 30 times a second. Only kinds that move need LIVE; for the others, send a new
  PICTURE when a setting changes.
- `s` is `model.settings(for: id)`, `id` is `model.programmerModule`.
- Missing values read as 0. A whole PICTURE stays under 640 bytes for every kind here (largest: reverb, 254).

## 1. LFO (kind 7), `LFODisplay`

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | waveform: `s.lfoWaveform.rawValue` (0 sine, 1 triangle, 2 saw, 3 square, 4 step, 6 random; 5 noise also draws) |
| 2 | v1 | depth: frac of `s.lfoDepth` (0..1) |
| 3 | v2 | cycles on the screen × 256: `max(rate × 2, 1) × 256`, `rate = s.lfoCyclesPerSecond(tempo: model.tempo)` (Rate 0.005..20 Hz, or Sync); clamp to 16383 |
| 4 | live0 | the scroll: `(t × rate) mod 8` × 2048, `t` the display clock in seconds (`Date().timeIntervalSinceReferenceDate`) |

No `arr`. Send LIVE at 30 fps while the LFO picture is up, so the wave runs past.

## 2. Envelope (kind 8), `EnvelopeDisplay`

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | attack: the knob's position, `log(attack / 0.001) / log(5000)` × 16383 (`s.attack`, 0.001..5 s) |
| 2 | v1 | decay: the same for `s.decay` |
| 3 | v2 | sustain: frac of `s.sustain` |
| 4 | v3 | release: the same as v0 for `s.release` |
| 5 | v4 | curve: frac of `s.envCurve` |

No `arr`, no LIVE.

## 3. Sequencer (kind 21), `StepNotesDisplay`

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | pattern length: `Int(s.seqLength)` (8..32) |
| 2 | v1 | the programmer's step cursor (`ToolScreen.stepIndex`, 0..31) |
| 3 | arr | 32 steps × 3 bytes from `model.playingSteps(of: id)` (padded to 32): note (0..127), flags, velocity (`round(velocity × 127)`) |
| 4 | live0 | the running step + 1: `model.playhead(of: id) + 1` (0: stopped) |

Flags byte: bit 0 `isOn`; bits 1-2 the gate's index in `SequencerStep.gates` (0: 0.25, 1: 0.5, 2: 0.75, 3: 1 = tie);
bits 3-4 `repeats - 1` (repeats 1..4). 96 bytes. Send a new PICTURE when a step changes, the cursor moves, or Mutate
swaps notes; send LIVE when the playhead moves.

## 4. Arp (kind 17), `ArpDisplay`

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | mode: `s.arp.mode.rawValue` (0 up, 1 down, 2 up/down, 3 played, 4 random) |
| 2 | v1 | octaves: `Int(s.arp.octaves.rounded())` (1..4) |
| 3 | v2 | gate: frac of `s.arp.gate` (0.05..1) |

## 5. Chord (kind 16), `ChordDisplay`

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | chord: `s.chord.rawValue` (0 off, 1 fifth, 2 octave, 3 minor, 4 major, 5 sus4, 6 minor 7, 7 major 7) |

The FM-1 has the semitones (`SynthChord.semitones`) built in.

## 6. MIDI (kind 19), `MidiDisplay`

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | channel: `Int(s.midiChannel.rounded())` (0 all, 1..16) |
| 2 | v1 | bend range in semitones: `Int(s.midiBend.rounded())` (0..24) |
| 3 | live0 | `model.isMidiActive`: 1 notes coming in, 0 not |

Send LIVE when `isMidiActive` changes.

## 7. Macro (kind 20), `MacroDisplay`

Drawn in the console look (black, grey type), as `ToolScreen` does for the macro.

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | `model.macroEdit.encoder` (0..3: A..D) |
| 2 | v1 | `model.macroEdit.step` (0..3; equal to the encoder's step count: the free slot) |
| 3 | v2 | flags: bit 0 the rack has changes not yet snapshotted |
| 4 | v3..v6 | step count of encoders A..D: `s.macro.encoders[i].steps.count` (0..4) |
| 5 | v7..v10 | position of encoders A..D: frac of `s.macro.encoders[i].position` |
| 6 | v11..v14 | cut mask of encoders A..D: bit n set when a choice switches at step n (step 0 never) |

No `arr`, no LIVE. The FM-1 draws the encoders side by side, each a tower of its steps from step 1 at the bottom;
a step with its cut bit gets a notch and no fade line into it.

## 8. Audio in (kind 18), `AudioInDisplay`

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | level: frac of `s.level` |
| 2 | v1 | `model.isAudioInputDenied`: 1 denied, 0 allowed |
| 3 | live0 | only while a cable modulates it: frac of `model.live(id, s.level, .oscLevel)`, drawn as a ghost bar (Steckfeld's modulated overlay) |

Without modulation send no LIVE (live_n 0).

## 9. Output (kind 0), `AmpDisplay`

No values, no `arr`: send a PICTURE with n 0 and an empty array when the output is selected. Each LIVE is one
meter reading; the FM-1 keeps the last 120 (four seconds at 30 a second) and draws them running past.

| # | Value | Mapping |
|---|---|---|
| 1 | live0 | the reading: frac of `MeterBallistics.next(peak).level` (0 = -48 dB .. 1 = 0 dB), `peak = model.takeOutputPeak()` |
| 2 | live1 | the held peak: frac of the same call's `.hold` |

Send it at 30 fps, as `AmpDisplay`'s timeline does, with the app's own `MeterBallistics`.

## 10. Bridge (kind 1), `BridgeDisplay`

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | input count: `model.inputs(of: id).count` (0..8) |
| 2 | v1 | output count: `model.connections.filter { $0.from == id }.count` (0..8) |
| 3 | v2.. | each input's weight, frac: `MixerTap(mixer: id.instance, index: i, count: inputs.count).weight(at: s.mixerBlend)`; send 16383 where there is no blend |
| 4 | arr | the input titles (`inputs[i].from.title`), then the output titles (`connection.to.title`), each a str, upper case |

The mode (OPEN, THRU, SPLIT, MERGE) follows from the counts as in `model.bridgeMode(_:)`. Names longer than 11
characters are cut. 8 inputs and 8 outputs with 20-character names fit in one message.

## 11. Delay (kind 9), `DelayDisplay`

The FM-1 runs the echoes itself (`DelayDisplay.lanes`), from:

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | time in ms: `s.delaySeconds(tempo: model.externalTempo ?? model.tempo) × 1000` (Time 0.02..2 s, or Sync), clamp 16383 |
| 2 | v1 | feedback: frac of `s.delayFeedback` (0..0.95) |
| 3 | v2 | mix: frac of `s.delayMix` |
| 4 | v3 | low cut in Hz: `s.delay.lowCut` (20..2000) |
| 5 | v4 | high cut in Hz: `s.delay.highCut` (500..18000), clamp 16383 |
| 6 | v5 | stereo: `s.delay.stereo.rawValue` (0 stereo, 1 ping-pong, 2 cross) |
| 7 | v6 | spread: frac of `s.delay.spread` |
| 8 | v7 | drive: frac of `s.delay.drive` |
| 9 | v8 | freeze: 1 when `s.delay.freeze`, else 0 |
| 10 | live0 | the sweep: `(t mod 4) / 4` × 16383, `t` the display clock in seconds |

Send LIVE at 30 fps while the delay picture is up.

## 12. Reverb (kind 10), `ReverbDisplay`

The tail comes from the app's engine, as `ReverbDisplay.tail` renders it (`synth_reverb_impulse`, 120 slices).

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | mix: frac of `s.reverbMix` |
| 2 | v1 | the window in seconds: the first of 1, 2, 5, 10, 20, 30 at least `span × 1.05`, `span = s.reverb.freeze ? 5 : s.reverb.decay + s.reverb.preDelay` |
| 3 | arr | 120 bytes for the left side, then 120 for the right: `round(127 × min(max(1 + dB / 60, 0), 1))`, `dB = 20 × log10(max(level, 1e-12) / peak)`, `peak` the loudest slice of both sides |

That is `ReverbDisplay`'s height without the mix factor: the FM-1 scales by v0, so a mix change needs only the
values (send the array again only when the tail changes). 240 bytes.

## What the FM-1 draws differently

- Modulation: only audio in shows the ghost of the modulated value. For the other kinds, send the modulated value
  in place of the set one if it should move.
- MIDI: the 16 channels are 2 rows of 8, not 4 × 4.
- Sequencer: no step numbers on the keys.
- Macro: the four encoders as towers of steps instead of rows; no dump, log or byte stream.
- Envelope: no A D S R labels; output and reverb labels use the FM-1's own type.
