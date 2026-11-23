# Changelog

All notable changes to Fernbedienung. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); each version goes with one Steckfeld SF1 version.

## [Unreleased]

## [1.2] - 2026-11-23

Goes with Steckfeld SF1 1.7.0 and newer.

### Added

- The crews' later modules get their own screens: BUBEN (the eight pads lit as they ring, the focused one bracketed),
  PRISE (the note heard in solfège, a tuner that says JUSTE or FAUX, the four voices), こだま KODAMA (the tape loop
  with its heads lit as they play, echo time, tape speed, the spring tank), MITSCHNITT (the take with Start, End, loop
  and voices, the record mode and the BACK buffer; the input level and the take filling up while armed or
  recording) and EMPFANG (station, line state,
  L / R meters, S1 AUS). RASTER's three modules are each their own drawing to DIN, as in Steckfeld: SCHLIERE a part
  drawing, MITSCHNITT a circuit diagram (lamps, potentiometer tiles), EMPFANG a polar diagram (polar tiles).
  BUBEN's top line is in VÝPIS's pixel font.
- KAIRO's screens are labelled in Japanese as in Steckfeld: こだま / 木霊 down KODAMA's side, its heads 消 録 一 二 三,
  頭, 強 and 自励, 速度 and 発条; 曲げ down MAGE's side and 位相 over its phase; 回路 KAIRO in the header. The glyphs
  come from the public-domain Shinonome fonts, as 1-bit bitmaps (tools/gen_kanji.py).

### Changed

- Macro: follows Steckfeld's new macro (STORE / STEP on REC / SAVE, one page). The header shows
  UNSAVED instead of the blinking ARMED. The picture shows the four encoders side by side, each with up to four
  steps stacked upwards, a line between written steps (a notch where a choice switches instead)
  and a pointer where the encoder stands.

## [1.1] - 2026-11-18

Goes with Steckfeld SF1 1.5.0 and newer. Tested on an FM-1 with an iPad.

### Added

- The hacker crews' modules (VÝPIS, COMBINÉ, 曲げ MAGE, SCHLIERE) get the whole screen in their crew's look: picture, tiles, header and footer.

### Changed

- README and the Firmware Hub card: the FM-1 is the full controller (every module can be edited from it without
  Steckfeld's programmer) and the hacker crews have their own screens.

### Fixed

- The screen fell further and further behind Steckfeld while a picture moved: the FM-1 took one message per drawn
  frame. It now takes every waiting message before it draws.
- Knob turns go out once a millisecond instead of once per drawn frame, so Steckfeld sees how fast a knob turns.

## [1.0] - 2026-11-16

Goes with Steckfeld SF1 1.4.0. Tested on an FM-1 with an iPad.

### Added

- Keys, knobs and buttons control Steckfeld over USB-MIDI; Steckfeld's screen comes back as SysEx
  ([protocol](docs/fernbedienung/protocol.md)).
- USB audio interface for the iPad (44.1 kHz, 8 channels in; Steckfeld plays into pair 1).
- Screens: Edit with module pictures, Route with cables, the patch list (patches that play a sequence marked SEQ),
  macro steps, notices, the FM-1's settings; in Steckfeld's display colours, blue by day, red in the dark.
- EDIT: MIDI channel, velocity and OPEN (Always: picking a module on the FM-1 opens Steckfeld's programmer; Never: it
  stays closed while the FM-1 is connected, even when a module is tapped in Steckfeld).
- System settings (hold HOME, press EDIT): theme Sync / Light / Dark.
- The update loader shows Elektro Kabelmaier's update screen while it writes.

[Unreleased]: https://github.com/DPangerl/fernbedienung/compare/v1.2...HEAD
[1.2]: https://github.com/DPangerl/fernbedienung/compare/v1.1...v1.2
[1.1]: https://github.com/DPangerl/fernbedienung/compare/v1.0...v1.1
[1.0]: https://github.com/DPangerl/fernbedienung/releases/tag/v1.0
