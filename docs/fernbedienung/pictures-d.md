<!-- SPDX-License-Identifier: CC0-1.0 -->
# Pictures, part D: the crews' later modules

What the app sends in PICTURE (cmd 20) and LIVE (cmd 21) for BUBEN, PRISE, こだま KODAMA, MITSCHNITT and EMPFANG.
The conventions are part B's ([pictures-b.md](pictures-b.md)). Like part C ([pictures-c.md](pictures-c.md)), the
FM-1 draws each in its crew's own look, not the display's theme (`firmware/apps/fernbedienung/fb_pics_d.h`): the
crew's ground, header and tiles, and a picture after the module's own art in Steckfeld. RASTER's three modules are
each a different sheet to DIN: SCHLIERE a part drawing (view A, a scale field), MITSCHNITT a circuit diagram (a
title block with the lamps H1 to H5; the tiles potentiometers R1 to R4, the wiper on the value), EMPFANG an
antenna's polar diagram (the lamp H1 and the bargraph instruments P1 and P2; the tiles polar grids, the value a
lobe).

## BUBEN (kind 26), ZKRAT

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | kit (0..3: KOTEL, PLECH, SKLEP, DRÁT) |
| 2 | v1 | the sound the knobs turn (0..7, kick first), `model.focusedSound(of:)` |
| 3 | v2 | volume: frac of `s.level` |
| 4 | v3 | drive: frac |
| 5 | v4 | feel: position of -1..1 (-1 loose, 0 straight, 1 swung) |
| 6 | arr | 8 bytes, one per sound: bits 0-1 choke group (0 none, 1..3), bit 2 set when the sound is turned away from the kit's own |
| 7 | live0..live7 | how loud each sound rings now: frac |

ALGORITHM (the programmer's PREV / NEXT) steps the sound the knobs turn. The FM-1 shows the kit, drive and feel on
the top line (feel as Steckfeld words it: STRAIGHT, SWING 63%, LOOSE 40), the eight pads with the key each sits on
(C D E F G A H #), lit as they ring, a dot on an edited one and a mark per choke group; the focused pad bracketed and
named under the pads.

## PRISE (kind 27), LIGNE

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | mode (0 SUIVRE, 1 ACCORD) |
| 2 | v1 | speed: position of 0..0.5 |
| 3 | v2 | direct: frac |
| 4 | v3 | level: frac |
| 5 | v4 | scale (0 CHROMA, 1 MAJEUR, 2 MINEUR, 3 PENTA) |
| 6 | v5 | key (0..11, DO first) |
| 7 | v6 | octave + 2 |
| 8 | v7 | threshold: position of -70..-20 dB |
| 9 | v8 | formant + 12 |
| 10 | v9 | drift: position of 0..30 |
| 11 | v10 | width: frac |
| 12 | v11 | voices (1..4) |
| 13 | live0 | the MIDI note it sends + 1, 0 while its gate is shut |
| 14 | live1 | the pitch heard, in hundredths of a semitone from MIDI's note 0; 0 silence |
| 15 | live2 | the input's level: frac of `min(level × 4, 1)` |
| 16 | live3..live6 | each voice's pitch as it sings, as live1 |
| 17 | live7..live10 | each voice's level: frac |

The FM-1 shows the note heard in solfège (French octaves: DO3 is MIDI's 60) and a tuner over a semitone that says
JUSTE within 20 cents, FAUX past that. It measures against the nearest note of the scale and key in SUIVRE, against
the nearest semitone in ACCORD (the notes held are not sent). SUIVRE also shows the note it sends; the four voices
run along the foot.

## こだま KODAMA (kind 28), KAIRO

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | head 1's echo in ms at the tempo (`s.kodama.seconds(tempo:)`) |
| 2 | v1 | while synced the note length + 1 (index in `ClockDivision.delay`: 1/2, 1/4, 1/4D, 1/4T, 1/8, 1/8D, 1/8T, 1/16, 1/16T, 1/32); 0 runs free |
| 3 | v2 | intensity: position of 0..1.5 |
| 4 | v3 | heads (0..6: 1, 2, 3, 1+2, 2+3, 1+3, 1+2+3) |
| 5 | v4..v7 | wow, flutter, wear, drive: frac |
| 6 | v8 | spring decay: log position of 0.5..6 s |
| 7 | v9..v11 | drip, spring tone, spring level: frac |
| 8 | v12, v13 | bass, treble: position of -12..12 dB |
| 9 | v14, v15 | echo, dry: frac |
| 10 | live0 | head 1 in ms as the tape runs now |
| 11 | live1 | speed off true: position of -1..1 % (half way is true) |
| 12 | live2 | record level: frac |
| 13 | live3..live5 | each playback head's level: frac, 0 for one not playing |
| 14 | live6 | the springs' level: frac |
| 15 | live7 | the tape's travel in head-1 spans, its fraction: frac |

The FM-1 labels it in Japanese as Steckfeld does: こだま and 木霊 down the side, the heads 消 (erase), 録 (record),
一 二 三 (playback), 頭 (heads), 強 (intensity), 速度 (speed), 発条 (the springs). Past 100 % intensity it says 自励 in
red: the tape howls by itself.

## MITSCHNITT (kind 29), RASTER

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | loop (0 off, 1 forward, 2 ping-pong) |
| 2 | v1 | warp (0 / 1) |
| 3 | v2 | root: MIDI note |
| 4 | v3 | level: frac |
| 5 | v4, v5 | start, end: frac of the take |
| 6 | v6, v7 | loop start, loop end: frac of start..end |
| 7 | v8 | crossfade: position of 0..0.5 |
| 8 | v9, v10 | fade in, fade out: log position of 0.001..2 s |
| 9 | v11 | the take's length in ms, 0 with none |
| 10 | v12 | threshold: position of -70..-6 dB (0: off, a take starts at once) |
| 11 | v13 | input gain: position of -24..24 dB |
| 12 | v14 | monitor: frac |
| 13 | v15 | record mode: 0 FWD, 1 FWD KEEP, 2 BACK, 3 BACK KEEP (`SamplerSettings.modes`). A take holds 12 s at most |
| 14 | arr | the take's outline, 128 peaks (7-bit) |
| 15 | live0 | state: 0 idle, 1 armed, 2 recording, 4 listening (BACK modes: the buffer runs, the take plays as idle) |
| 16 | live1 | ms recorded so far; listening: ms buffered |
| 17 | live2 | the input's peak: frac |
| 18 | live3..live6 | each voice's place in the take: frac, 0 for one not playing |
| 19 | live7..live10 | each voice's gain: frac, 0 for one not playing |

REC records or stops (its label in SCREEN: REC / STOP; GRAB in the BACK modes, where it takes what the buffer
holds). The FM-1 labels in German as RASTER's sheet does: BEREIT, LEER, PUFFER n s (listening), SCHARF (armed,
blinking), AUFNAHME, with the record mode on the right. Idle or listening it shows the take with Start (A) and End
(E), the loop bracketed under it and a line per voice playing, and along the foot the root, the loop (SCHLEIFE,
PENDEL) and the take's length; armed or recording the input's peak from -60 dB against the threshold and the take
filling up towards 12 s. The loop is dimensioned under the take: one arrowhead forward, two ping-pong. The title
block along the foot holds the root, the take's length (recording: the seconds so far; armed: the threshold it waits
for) and the lamps: H1 red while recording, blinking while armed, H2 to H5 one per voice, gold as loud as it plays.

## EMPFANG (kind 30), RASTER

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | station: index in `RadioStation.all` |
| 2 | v1 | stations: `RadioStation.all.count` |
| 3 | v2 | volume: frac of `s.level` |
| 4 | v3 | S1, the rotary switch: 1 EIN, 0 AUS (the line hung up). Missing reads as EIN |
| 5 | arr | the station's name str, then its genre str |
| 6 | live0 | the line: 0 off, 1 dialling, 2 live, 3 dialling again, 4 no network |
| 7 | live1 | 1 while audio plays |
| 8 | live2, live3 | left and right level before the volume: frac |
| 9 | live4 | ms buffered |

The FM-1 shows the line's lamp H1 (gold steady while it plays, blinking while it dials, red with no network) with
LIVE, WÄHLT / WÄHLT NEU (blinking), KEIN NETZ or AUS, SENDER n VON 9, the name large, the genre, the buffer, and the
bargraph instruments P1 L and P2 R: twelve cells of 4 dB from -48 dB, the last two red. At S1 AUS the name dims and S1 AUS replaces the meters.
