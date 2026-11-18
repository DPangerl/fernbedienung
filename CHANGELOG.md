# Changelog

All notable changes to Fernbedienung. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); each version goes with one Steckfeld SF1 version.

## [Unreleased]

## [1.1] - 2026-11-18

Goes with Steckfeld SF1 1.5.0 and newer. Tested on an FM-1 with an iPad.

### Added

- The hacker crews' modules (VÝPIS, COMBINÉ, 曲げ MAGE, SCHLIERE) get the whole screen in their crew's look: picture, tiles, header and footer.

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

[Unreleased]: https://github.com/DPangerl/fernbedienung/compare/v1.1...HEAD
[1.1]: https://github.com/DPangerl/fernbedienung/compare/v1.0...v1.1
[1.0]: https://github.com/DPangerl/fernbedienung/releases/tag/v1.0
