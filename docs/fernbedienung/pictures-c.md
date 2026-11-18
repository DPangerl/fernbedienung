<!-- SPDX-License-Identifier: CC0-1.0 -->
# Pictures, part C: the hacker crews' modules

What the app sends in PICTURE (cmd 20) and LIVE (cmd 21) for the four crew modules. The conventions are part B's
([pictures-b.md](pictures-b.md)). The FM-1 draws each in its crew's own look, not the display's theme
(`firmware/apps/fernbedienung/fb_pics_c.h`).

## VÝPIS (kind 22), ZKRAT

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | address: frac of `s.dump.address` |
| 2 | v1 | length: index in `DumpSettings.lengths` (0..4: 16, 32, 64, 128, 256 bytes) |
| 3 | v2 | glitch: frac |
| 4 | v3 | level: frac |
| 5 | arr | the cycle's bytes, each `>> 1` (`DumpView(settings:).bytes`) |
| 6 | live0, live1 | address (wrapped to 0..1) and level as cables have them, only while modulated |

The FM-1 shows the address as the hex dump prints it (`$%04X` of the line, 16 bytes a line), the memory map, the
cycle's top seven bits per byte, the stepped level and a read head that walks through the cycle every two seconds.

## COMBINÉ (kind 23), LIGNE

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | bands (2..8) |
| 2 | v1 | crackle: frac |
| 3 | v2 | tone: frac |
| 4 | live0..live7 | each band's level, lowest first: frac of `min(level × 4, 1)` |

## 曲げ MAGE (kind 24), KAIRO

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | shape (0 saw, 1 square, 2 pulse, 3 reso) |
| 2 | v1 | bend: frac |
| 3 | v2 | level: frac |
| 4 | live0, live1 | bend and level as cables have them, only while modulated |

## SCHLIERE (kind 25), RASTER

| # | Value | Mapping |
|---|---|---|
| 1 | v0 | glide: log position of 0.01..4 s |
| 2 | v1 | saws (1..7) |
| 3 | v2 | spread: frac |
| 4 | v3 | level: frac |
| 5 | live0 | level as cables have it, only while modulated |
