# MK47 MIDI

QMK firmware for the Inland MK47 as a MIDI controller: a chromatic and a bass
guitar layout, a looper with overdub and undo, lighting that responds to
incoming MIDI (for Resolume and similar), and piano-key / fret-marker guide
lighting.

The layout, every shift-layer function and the lighting are documented in the
keymap's own readme:
[keyboards/inland/mk47/keymaps/midiboard/readme.md](keyboards/inland/mk47/keymaps/midiboard/readme.md)

## Building

This is a QMK external userspace: it holds only the keymap and builds against
a separate QMK checkout.

- **QMK version:** built and tested against upstream QMK at commit
  `4e04da397e` (21 Feb 2024). Newer QMK may need changes.
- **QMK checkout location:** `~/qmk_firmware` by default; override with
  `make build QMK_HOME=/path/to/qmk_firmware`.
- **Patch:** `patches/midi-input-not-discarded.patch` fixes a bug in that QMK
  version that throws away all incoming MIDI. `make build` applies it to the
  QMK checkout automatically if it is missing.
- **Tools:** the QMK build environment (`arm-none-eabi-gcc`), plus
  `wb32-dfu-updater_cli` for flashing (`brew install wb32-dfu-updater_cli`).

```sh
make build    # writes inland_mk47_midiboard.bin to this folder
make flash    # builds, then flashes; keyboard must be in bootloader mode
```

Bootloader mode: hold the top-left key while plugging in the keyboard.

Prebuilt firmware: download `inland_mk47_midiboard.bin` from the
[latest release](https://github.com/gghostdadd/mk47-midi/releases/latest) and
flash it with [QMK Toolbox](https://qmk.fm/toolbox), no build environment needed.

## Testing the MIDI lighting

`tools/mk47test.swift` sends note-ons to the keyboard from macOS:

```sh
swift tools/mk47test.swift 60 5 64 21    # note 60 red, note 64 green
swift tools/mk47test.swift sweep         # every note shows its palette colour
swift tools/mk47test.swift clear         # velocity 0 to every note (channel 1)
```

## License

GPL-2.0-or-later, matching QMK. See [LICENSE](LICENSE).
