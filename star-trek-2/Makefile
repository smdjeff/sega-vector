## toolchain
# export PATH=${PATH}:/Users/jeff/Desktop/z88dk/bin
# export ZCCCFG=/Users/jeff/Desktop/z88dk/lib/config
# see https://github-wiki-see.page/m/z88dk/z88dk/wiki/NewLib--Platform--Embedded
#
## bootloader rom
# development is made easier by loading bootrom into 1873.cpu-u25
# then romulator can be quickly loaded to a single large rom on the ROM board
# patch sdcc for crt_page_zero (so our bootloader rom works)
# see https://www.z88dk.org/forum/viewtopic.php?t=12010
#
## rom programming
# ~/Downloads/dasmxx-master/src/dasmz80 dasmxx/sega.txt > dasmxx/gamerom.asm
# minipro -p M2716@DIP24 -w /Users/jeff/Downloads/bootrom.bin  --vpp=25
#
## hardware flashing
# ROM Emulator https://github.com/Kris-Sekula/EPROM-EMU-NG/
# python3 -m pip install serial
# python3 -m pip install PySimpleGUI
# brew install python-tk

# --- Build directories
BUILD_DIR        := build
ART_BUILD_DIR    := $(BUILD_DIR)/art
SPEECH_BUILD_DIR := $(BUILD_DIR)/speech

# --- Art
SVGS_ROOT := $(wildcard art/*.svg)
SVGS_FONT := $(wildcard art/font/*.svg)
HDRS_ROOT := $(patsubst art/%.svg, $(ART_BUILD_DIR)/%.h, $(SVGS_ROOT))
HDRS_FONT := $(patsubst art/font/%.svg, $(ART_BUILD_DIR)/%.h, $(SVGS_FONT))
HDRS      := $(HDRS_ROOT) $(HDRS_FONT)

# --- Speech 
SPEECH_SOURCES := $(wildcard speech/*.wav)
LPC_TARGETS    := $(patsubst speech/%.wav, $(SPEECH_BUILD_DIR)/%.lpc, $(SPEECH_SOURCES))

# --- Compiler & Tool Definitions ---
MAME   = ~/Downloads/mame-master/mame
SOURCE = main.c font.c math.c usb.c vector.c game-crystal.c game-shirt.c game-ferengi.c

CFLAGS = -DENABLE_BOOTROM
CFLAGS += -DMAME_BUILD
CFLAGS += -I$(ART_BUILD_DIR)/..

# --- Main Targets

all: gamerom

.PHONY: all art speech clean prereq flash mame roms bootrom

prereq:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(ART_BUILD_DIR)
	@mkdir -p $(SPEECH_BUILD_DIR)
	@mkdir -p roms

clean:
	$(RM) -rf $(BUILD_DIR)
	$(RM) -rf roms

# --- Compilation Targets

gamerom: prereq bootrom speech art
	@echo "building 48k rom 0x0800-0xBFFF, sega-boot-rom in cpu rom socket"
	zcc +z80 --list -m -vn -SO3 -compiler=sdcc -startup=1 $(SOURCE) -o $@ -create-app $(CFLAGS) -Cz"--rombase=0x0800 --romsize=47104"
	@mv $@* build/ 2>/dev/null || true
	@mv *.lis build/ 2>/dev/null || true
	@printf 'ROM usage: ' && stat -f '%z' build/$@_CODE.bin
	@printf 'RAM usage: ' && echo "$$(stat -f '%z' build/$@_BSS.bin) + $$(stat -f '%z' build/$@_DATA.bin)" | bc
	@grep -q "/crt_page_zero_z80.inc:" build/$@.lis || (echo "ERROR crt_page_zero missing. please edit z80_crt_o.asm.m4" && exit 1)
	@rm -f build/$@.bin
	cat build/bootrom.bin >> build/$@.bin
	cat build/$@.rom >> build/$@.bin
	truncate -s 48K build/$@.bin

bootrom: prereq
	z80asm bootrom.asm -o build/$@.bin
	truncate -s 2K build/$@.bin

# --- ROM generation

roms: prereq gamerom
	@cp build/bootrom.bin roms/1873.cpu-u25
	@split -b 2048 build/gamerom.bin roms/part_
	@i=0; \
	for f in $$(ls roms/part_* | sort); do \
		if [ "$$(crc32 "$$f")" = "3f55d17f" ]; then \
			rm "$$f"; \
		else \
			mv "$$f" "roms/$$(($$i + 1847)).prom-u$$i"; \
		fi; \
		i=$$(($$i + 1)); \
	done
	@rm roms/1847.prom-u0

mame: roms
	# effectively a patch over the existing to make mame happy
	$(RM) -rf startrek
	unzip startrek.zip -d startrek
	# bring in changes
	rsync -vt roms/* startrek 
	# new speech data
	cp $(SPEECH_BUILD_DIR)/rom_0.bin startrek/1871.speech-u6
	cp $(SPEECH_BUILD_DIR)/rom_1.bin startrek/1872.speech-u5
	cp $(SPEECH_BUILD_DIR)/rom_2.bin startrek/187x.speech-u4
	cp $(SPEECH_BUILD_DIR)/rom_3.bin startrek/187x.speech-u3
	# new 8035 speech mcu driver
	cp ../speech/main.bin startrek/1670.speech-u7
	zip roms.zip startrek/*
	$(RM) -rf startrek
	mv roms.zip build/startrek.zip
	$(MAME) -rp ./build startrek

# --- art 

art: prereq $(HDRS)

$(ART_BUILD_DIR)/logo.h: art/logo.svg
	../tools/svg2sega.py --scale 2.0 --rotate 90 --speed 0 --origin-left --merge 1.0 --rdp 0 --no-opt --close $< > $@

$(ART_BUILD_DIR)/shirt%.h: art/shirt%.svg
	../tools/svg2sega.py --scale 0.7 --rotate 90 --speed 0 --rdp 4.0 --close $< > $@

$(ART_BUILD_DIR)/crystal%.h: art/crystal%.svg
	../tools/svg2sega.py --scale 0.6 --rotate 90 --speed 0 --rdp 1.0 --close $< > $@

$(ART_BUILD_DIR)/%.h: art/%.svg
	../tools/svg2sega.py --scale 0.6 --rotate 90 --speed 0 --rdp 2.0 --close $< > $@

$(ART_BUILD_DIR)/%.h: art/font/%.svg
	../tools/svg2sega.py --scale 0.4 --rotate 90 --speed 0 --origin-top-left --close $< > $@

# --- speech 

speech: prereq $(LPC_TARGETS)
	@echo "Packing LPC files into 4K chunks..."
	python3 ../tools/pack_lpc.py $(SPEECH_BUILD_DIR) $(SPEECH_BUILD_DIR)/rom

$(SPEECH_BUILD_DIR)/%.lpc: speech/%.wav
	../tools/wav2lpc.py $< $@
	../tools/lpc2wav.py $@ $(@:.lpc=.wav) --quiet
	@afplay $(@:.lpc=.wav)

# --- Hardware Programming

PROGRAMMER = python3 ~/Downloads/EPROM_EMU_NG_2.0rc9.py
UART = /dev/cu.usbserial-B00050RO

flash:
	 ${PROGRAMMER} -mem 27512 -spi y -auto y -start 0 build/gamerom.bin ${UART}
