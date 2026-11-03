# QMK external userspace for the Inland MK47 (WB32FQ95, wb32-dfu bootloader).
# Builds against the upstream QMK checkout in ~/qmk_firmware.
QMK_HOME ?= $(HOME)/qmk_firmware
KEYMAP ?= midiboard
BIN := inland_mk47_$(KEYMAP).bin

PATCH := $(CURDIR)/patches/midi-input-not-discarded.patch

.PHONY: build clean flash patch pages-firmware

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

# Copy the built firmware into the GitHub Pages site, where the web flasher
# (docs/flash.html) offers it as the latest version. Run it when making a
# release, with the release's tag: make pages-firmware VERSION=v1.1.0
pages-firmware: build
	@test -n "$(VERSION)" || { echo "Set VERSION to the release tag, e.g. make pages-firmware VERSION=v1.1.0"; exit 1; }
	mkdir -p docs/firmware
	cp "$(BIN)" docs/firmware/
	printf '{"version": "%s", "file": "%s", "size": %s, "sha256": "%s"}\n' "$(VERSION)" "$(BIN)" \
		"$$(wc -c < "$(BIN)" | tr -d ' ')" "$$(shasum -a 256 "$(BIN)" | cut -d ' ' -f 1)" > docs/firmware/latest.json
