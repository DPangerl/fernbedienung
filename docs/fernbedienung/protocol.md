<!-- SPDX-License-Identifier: CC0-1.0 -->
# Fernbedienung protocol, version 1

How the FM-1 running Fernbedienung and Steckfeld SF1 talk over USB-MIDI. This document is public domain (CC0), so
any app can implement it without taking on the firmware's licence. The constants are in
`firmware/apps/fernbedienung/fb_proto.h`.

An FM-1 firmware version is built for one protocol version. When the versions differ, the FM-1 shows that it
needs an update; the app does not have to stay compatible with older versions.

## Who does what

The FM-1 has no idea of modules or patches. It sends what the hands do, and draws what the app sends. The app
decides which module is selected, what the knobs move, and what the screen shows.

## FM-1 → app: controls

Channel messages on the MIDI channel set on the FM-1 (EDIT page, default 1). Encoders send relative values, two's
complement: 1 is one detent up, 127 one down, 2 / 126 two detents, and so on.

| # | Control | Message | What the app does with it |
|---|---|---|---|
| 1 | KNOB1-4 | CC 20-23, relative | the 4 values of the page |
| 2 | SELECT | CC 24, relative | the module's pages (tabs) |
| 3 | ALGORITHM | CC 25, relative | exactly the programmer's PREV / NEXT keys: the cable on Route, the step in the sequencer, BUBEN's sound. Elsewhere nothing |
| 4 | PRESETS | CC 26, relative | opens the patch list and browses it (LIST); nothing loads yet. While the list is open, SELECT steps the library's category filter (all, bass, keys, lead, pad, strings, pluck, FX, perc) |
| 5 | REC | CC 27, 127 pressed, 0 released | while a NOTICE with an action shows: that action. Else the page's action (its label is in SCREEN): load the patch while the list is open, step on/off, macro store, freeze, MITSCHNITT's recording (REC / STOP, GRAB in its BACK modes), panic, cable on/off |
| 6 | SAVE | CC 28, 127 / 0 | the page's second action (label in SCREEN): macro step, cable flip |
| 7 | category button tapped | CC 40 + category, 127 | not on that category: go to it (the module used there last). Already on it: switch Edit / Route |
| 8 | category button held + SELECT | CC 50 + category, relative | step through the modules of that category |
| 9 | PLAY | CC 29, 127 pressed, 0 released | the app starts or stops its own transport. When it follows an external clock it leaves the transport to that master and shows a NOTICE ("Clock: external"). The PLAY LED follows SCREEN's running flag |
| 10 | keys, OCT | note on / off | notes; in a sequencer's Edit screen the app takes them as step entry (below) |

A tap is a press and release with no SELECT turn in between. MASTER is the FM-1's own output volume and sends
nothing. EDIT opens the FM-1's own settings and sends nothing.

Categories and the module kinds the app puts in each:

| # | Category | Button | Kinds |
|---|---|---|---|
| 0 | HOME | HOME | macro; with no macro in the bay, the output (amp), so HOME always shows something |
| 1 | SCL | SCL | oscillator, wavetable, FM, string, grain, VÝPIS, 曲げ MAGE, SCHLIERE, BUBEN, PRISE, MITSCHNITT, EMPFANG |
| 2 | FX | FX | filter, delay, reverb, distortion, chorus, EQ, compressor, COMBINÉ, こだま KODAMA |
| 3 | LFO | LFO | LFO |
| 4 | ENV | ENV | envelope |
| 5 | SEQ | SEQ | sequencer |
| 6 | ARP | ARP | arp, chord |
| 7 | GLO | GLO | MIDI, audio in, bridge, output |

Sequencer step entry (Edit screen): ALGORITHM moves the step cursor (the programmer's PREV / NEXT). A key on an empty step puts that note in, a
different key changes the step's note, the same key again clears the step.

Patch list: PRESETS opens it and browses; SELECT steps the category filter; REC loads the selected patch (it
replaces the whole bay); any other control closes the list without loading. The FM-1 also takes the list off its
screen at once on any other control; the app still sends LIST_CLOSE.

The FM-1 never changes the app's settings. When something stands in the way (the app plays to another output, it
follows an external clock) the app sends a NOTICE; with an action, REC does it ("play through FM-1"), so in a
bigger setup the user can leave things as they are.

## SysEx

`F0 7D 46 42 <cmd> <body> F7`. `7D` is MIDI's non-commercial id, `46 42` is "FB". Every body byte is 7-bit.

- A field is one byte (0..127) unless it says v14 or str.
- **v14**: a 14-bit value as two bytes, high 7 bits first (0..16383).
- **str**: ASCII 0x20-0x7E, ended by 0x00. The FM-1 draws upper case as sent.
- A SysEx message is at most 640 bytes from F0 to F7.

### FM-1 → app

| cmd | Name | Body |
|---|---|---|
| 01 | HELLO | protocol version, firmware major, firmware minor, flags (bit 0: never open the programmer), theme (0 follow the app, 1 light, 2 dark) |

The FM-1 sends HELLO once USB is up, when the app asks (ASK_HELLO) and whenever one of its settings changes. The app answers with
ACK and then sends the full state: SCREEN, CONTROLS, PICTURE.

### App → FM-1

| cmd | Name | Body |
|---|---|---|
| 02 | ACK | protocol version, theme (1 light, 2 dark: the app's own) |
| 10 | SCREEN | category, mode (0 Edit, 1 Route), kind (below), page, page count, flags (bit 0: the macro has changes not yet snapshotted, set only while the macro shows; bit 1: transport running), title str, page count × page title str, REC label str, SAVE label str (empty: the button does nothing here) |
| 11 | CONTROLS | 4 × control: flags (bit 0 present, bit 1 live value follows), position v14, live v14 if bit 1, title str, readout str |
| 20 | PICTURE | kind, n, n × v14, array length v14, array bytes (7-bit). Layout per kind: [pictures-a.md](pictures-a.md), [pictures-b.md](pictures-b.md), [pictures-c.md](pictures-c.md), [pictures-d.md](pictures-d.md) |
| 21 | LIVE | kind, n, n × v14: what moves (playhead, peaks, modulated values), up to 30 times a second |
| 12 | NOTICE | text str (empty: none; at most 18 characters, e.g. "Sound on iPad", "Clock: external"), action str (what REC does about it, at most 10, e.g. "use FM-1"; empty: nothing). Shown in the footer, amber, until replaced |
| 13 | BAY | categories v14: bit n set, category n (the table above) has a module in the bay. Sent with the full state and whenever that changes. The FM-1 turns off the backlight of the buttons of empty categories; until BAY comes, every category counts as full |
| 30 | LIST | type (0 cables, 1 patches), selected v14, first v14, count, title str (the patch list's category filter, e.g. "BASS"; empty for cables), count × row: flags (bit 0 on, bit 1 outgoing, bit 2 patches: plays a sequence, not only MIDI in; the FM-1 shows SEQ), text str, detail str |
| 31 | LIST_CLOSE | (none): closes the patch list |
| 7F | ASK_HELLO | (none): the app has started and wants to know who is there |

Positions (v14) run 0..16383 over the control's sweep, the way the programmer's ring shows it. LIST rows: the app
sends a window of rows around the selection (`first` is the index of the first row sent), as many as fit in one
message.

Module kinds:

| # | Kind | # | Kind | # | Kind |
|---|---|---|---|---|---|
| 0 | output | 8 | envelope | 16 | chord |
| 1 | bridge | 9 | delay | 17 | arp |
| 2 | oscillator | 10 | reverb | 18 | audio in |
| 3 | wavetable | 11 | distortion | 19 | MIDI |
| 4 | string | 12 | chorus | 20 | macro |
| 5 | FM | 13 | EQ | 21 | sequencer |
| 6 | filter | 14 | compressor | 22 | VÝPIS (dump) |
| 7 | LFO | 15 | grain | 23 | COMBINÉ |
| | | | | 24 | 曲げ MAGE |
| | | | | 25 | SCHLIERE (smear) |
| | | | | 26 | BUBEN (drum) |
| | | | | 27 | PRISE |
| | | | | 28 | こだま KODAMA |
| | | | | 29 | MITSCHNITT (sampler) |
| | | | | 30 | EMPFANG (radio) |

## The FM-1's own settings (EDIT)

MIDI channel 1-16, velocity, OPEN. The theme (follow the app, light, dark) is in the FM-1's system settings (hold
HOME, press EDIT). They are sent in HELLO.

- OPEN Always (flag clear): selecting a module on the FM-1 always opens the app's programmer on it.
- OPEN Never (flag set): while the FM-1 is connected the app never opens its programmer, not even when a module is
  tapped in the app.
- Either way, a module selected in the app is selected on the FM-1 too: the app sends SCREEN, CONTROLS and PICTURE
  for it right away.
