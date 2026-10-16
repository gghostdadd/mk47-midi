# QMK external userspace for the Inland MK47 (WB32FQ95, wb32-dfu bootloader).
# Builds against the upstream QMK checkout in ~/qmk_firmware.
QMK_HOME ?= $(HOME)/qmk_firmware
KEYMAP ?= midiboard
BIN := inland_mk47_$(KEYMAP).bin

PATCH := $(CURDIR)/patches/midi-input-not-discarded.patch

.PHONY: build clean flash patch

# Apply our QMK core patch unless it is already applied. Without it this QMK
# version throws away incoming MIDI, so host-controlled lighting cannot work.
patch:
	@git -C "$(QMK_HOME)" apply --reverse --check "$(PATCH)" 2>/dev/null || git -C "$(QMK_HOME)" apply "$(PATCH)"

build: patch
	$(MAKE) -C "$(QMK_HOME)" inland/mk47:$(KEYMAP) QMK_USERSPACE="$(CURDIR)"

clean:
	$(MAKE) -C "$(QMK_HOME)" clean QMK_USERSPACE="$(CURDIR)"

# Put the keyboard in DFU first: hold the top-left key while plugging in USB.
flash: build
	wb32-dfu-updater_cli --dfuse-address 0x08000000 --download "$(BIN)"
	wb32-dfu-updater_cli --reset
