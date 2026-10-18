# Inland MK47 - `midiboard` keymap

A MIDI keyboard layout with a built-in loop sequencer, lighting that responds to
incoming MIDI, and additional lighting options.

The space bar is the ENTER key on startup (also handy for how I use Resolume). To change it to SPACE see below. 

Build and flash from the project root: `make build`, then `make flash` with the
keyboard in bootloader mode (hold the top-left key while plugging in).

QMK Toolbox is also handy if you'd like to flash the prebuilt .bin file: https://qmk.fm/toolbox
Hold the top-left key while plugging in and select MK47 in QMK Toolbox to flash firmware file.  

The latest prebuilt `inland_mk47_midiboard.bin` is on the releases page: https://github.com/gghostdadd/mk47-midi/releases/latest

Keyboard Layout:

## Layer 0 - Chromatic layout (default)

Each row is one octave, C to B. Lowest octave at the bottom.

```
+------+------+------+------+------+------+------+------+------+------+------+------+
|  C4  | C#4  |  D4  | D#4  |  E4  |  F4  | F#4  |  G4  | G#4  |  A4  | A#4  |  B4  |
+------+------+------+------+------+------+------+------+------+------+------+------+
|  C3  | C#3  |  D3  | D#3  |  E3  |  F3  | F#3  |  G3  | G#3  |  A3  | A#3  |  B3  |
+------+------+------+------+------+------+------+------+------+------+------+------+
|  C2  | C#2  |  D2  | D#2  |  E2  |  F2  | F#2  |  G2  | G#2  |  A2  | A#2  |  B2  |
+------+------+------+------+------+------+------+------+------+------+------+------+
|  C1  | C#1  |  D1  | D#1  |  E1  | ENTER/SPACE |  G1  | G#1  |  A1  | A#1  |SHIFT |
+------+------+------+------+------+-------------+------+------+------+------+------+
```

## Layer 1 - Bass guitar layout (toggle with SHIFT + BASS)

Each row is a fourth (5 semitones) above the one below, like bass strings E A D G.
Enter key becomes A.  

```
+------+------+------+------+------+------+------+------+------+------+------+------+
|  G2  | G#2  |  A2  | A#2  |  B2  |  C3  | C#3  |  D3  | D#3  |  E3  |  F3  | F#3  |
+------+------+------+------+------+------+------+------+------+------+------+------+
|  D2  | D#2  |  E2  |  F2  | F#2  |  G2  | G#2  |  A2  | A#2  |  B2  |  C3  | C#3  |
+------+------+------+------+------+------+------+------+------+------+------+------+
|  A1  | A#1  |  B1  |  C2  | C#2  |  D2  | D#2  |  E2  |  F2  | F#2  |  G2  | G#2  |
+------+------+------+------+------+------+------+------+------+------+------+------+
|  E1  |  F1  | F#1  |  G1  | G#1  |      A1     |  B1  |  C2  | C#2  |  D2  |SHIFT |
+------+------+------+------+------+-------------+------+------+------+------+------+
```

## Layer 2 - Shift (hold the bottom-right key)

Blank keys do nothing extra here: they still play their note. While shift is
held, the function keys light up in colors.

```
+------+------+------+------+------+------+------+------+------+------+------+------+
| CH1  | CH2  | CH3  | CH4  | CH5  | CH6  | CH7  | CH8  | CH9  | CH10 | BASS |RGBMOD|
+------+------+------+------+------+------+------+------+------+------+------+------+
| HUE+ | CH-  | CH+  |      |      |      |LEDOFF| FX-  | FX+  | BRI- | BRI+ |LIGHT |
+------+------+------+------+------+------+------+------+------+------+------+------+
|LSEM- |LSEM+ |LOCT- |LOCT+ |      |ENT/SP|      |      |      |      |      |      |
+------+------+------+------+------+------+------+------+------+------+------+------+
|KOCT- |KOCT+ | HALF |DOUBLE|CLEAR |    PLAY     |      |      | UNDO |REC/OV|(held)|
+------+------+------+------+------+-------------+------+------+------+------+------+
```

## Shift layer key

### MIDI

| CH1-CH10 | Change controller to that MIDI channel |
<br>| CH- / CH+ | Step the MIDI channel down / up |
<br>| KOCT- / KOCT+ | Shift the whole keyboard down / up an octave |
<br>| BASS | Switch between chromatic and bass guitar layouts |
<br>| ENT/SP | Switch the wide key on the chromatic layout between Enter and Space. Starts as Enter when plugged in; the key lights teal for Enter, yellow for Space while shift is held. |

### Looper

| REC/OV | Record / overdub. No loop: arm, recording starts on your first note; press SHIFT and REC/OV again to loop it. 
<br>  While loops is playing, hit record again for punch in / out of overdub. 
<br>  Loop recorded but stopped: start it with overdub on.
  
<br>| PLAY | Stop the loop / start it again from the top |
<br>| CLEAR | Stop and erase the loop |
<br>| UNDO | Remove the most recent overdub (repeat for earlier ones) |
<br>| HALF / DOUBLE | Halve / double the loop speed (up to 5 times each way) |
<br>| LOCT- / LOCT+ | Transpose the loop down / up an octave |
<br>| LSEM- / LSEM+ | Transpose the loop down / up a semitone |

### Lights

| LEDOFF | Clear every color set over MIDI, on all channels (for if lights are left hanging in Resolume) |
<br>| LIGHT | Toggle guide lighting. Chromatic layout: white keys lit, black keys dark, C in blue. Bass guitar layout: fret dots (see below). |
<br>| RGBMOD | Next RGB mode: dark mode first (midi in sets lighting), then the built-in pretty color effects. |
<br>| HUE+ | Shift the RGB mode effect color |
<br>| BRI- / BRI+ | LED Brightness down / up |
<br>| FX- / FX+ | RGB mode effect animation speed down / up |

## Guide lighting on the chromatic layout

With guide lighting on, the chromatic layout lights like a piano: `*` is a
white key (lit white), `C` is every C (lit blue, so octaves are easy to find)
and blank keys are the black keys (dark). Enter and shift stay dark.

```
 note:  C      C#     D      D#     E      F      F#     G      G#     A      A#     B
     +------+------+------+------+------+------+------+------+------+------+------+------+
  4  |  C   |      |  *   |      |  *   |  *   |      |  *   |      |  *   |      |  *   |
     +------+------+------+------+------+------+------+------+------+------+------+------+
  3  |  C   |      |  *   |      |  *   |  *   |      |  *   |      |  *   |      |  *   |
     +------+------+------+------+------+------+------+------+------+------+------+------+
  2  |  C   |      |  *   |      |  *   |  *   |      |  *   |      |  *   |      |  *   |
     +------+------+------+------+------+------+------+------+------+------+------+------+
  1  |  C   |      |  *   |      |  *   |   (enter)   |  *   |      |  *   |      |SHIFT |
     +------+------+------+------+------+-------------+------+------+------+------+------+
```

## Guide lighting on the bass guitar layout

Each column is a fret, with the open strings in the left column, so the guide
lighting marks it like a bass neck: `o` is an open string (amber) and `*` is a
fret dot (white) at frets 3, 5, 7 and 9.

```
 fret:  0      1      2      3      4      5      6      7      8      9     10     11
     +------+------+------+------+------+------+------+------+------+------+------+------+
  G  |  o   |      |      |  *   |      |  *   |      |  *   |      |  *   |      |      |
     +------+------+------+------+------+------+------+------+------+------+------+------+
  D  |  o   |      |      |  *   |      |  *   |      |  *   |      |  *   |      |      |
     +------+------+------+------+------+------+------+------+------+------+------+------+
  A  |  o   |      |      |  *   |      |  *   |      |  *   |      |  *   |      |      |
     +------+------+------+------+------+------+------+------+------+------+------+------+
  E  |  o   |      |      |  *   |      |      *      |  *   |      |  *   |      |SHIFT |
     +------+------+------+------+------+-------------+------+------+------+------+------+
```

## Bottom-right SHIFT key light (looper status)

| Blinking red | Armed, waiting for the first note |
<br>| Solid red | Recording or overdubbing |
<br>| Green | Loop playing |
<br>| Amber | Loop stopped, still in memory |
<br>| Dark | No loop |

## Lighting keys from MIDI input

A note-on sent to the keyboard lights every key currently assigned that note.
The velocity of that note output picks the color from the Launchpad 128-color palette.  Velocity 0
turns it off, and note-offs are ignored. Colors are kept per MIDI channel and
shown for the channel the keyboard is sending on.

## Notes

- Note names above are QMK's, as written in `keymap.c`. They sit lower than the
  usual naming: QMK's C1 is MIDI note 60 (middle C) at the default octave.
- This file is written by hand. It will need updating if the layers in `keymap.c` change.
