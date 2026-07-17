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

# --- Art
SVGS_ROOT := $(wildcard art/*.svg)
SVGS_FONT := $(wildcard art/font/*.svg)
HDRS_ROOT := $(patsubst art/%.svg, $(ART_BUILD_DIR)/%.h, $(SVGS_ROOT))
HDRS_FONT := $(patsubst art/font/%.svg, $(ART_BUILD_DIR)/%.h, $(SVGS_FONT))
HDRS      := $(HDRS_ROOT) $(HDRS_FONT)

# --- Compiler & Tool Definitions ---
MAME   = ~/Downloads/mame-master/mame
SOURCE = main.c font.c math.c usb.c vector.c game-planets.c

CFLAGS = -DENABLE_BOOTROM
CFLAGS += -DMAME_BUILD
CFLAGS += -I$(ART_BUILD_DIR)/..

# --- Main Targets

all: build/gamerom.bin

.PHONY: all art clean prereq flash mame roms

prereq:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(ART_BUILD_DIR)
	@mkdir -p roms

clean:
	$(RM) -rf $(BUILD_DIR)
	$(RM) -rf roms

# --- Compilation Targets

build/gamerom.bin: build/bootrom.bin $(SOURCE) sega.h $(HDRS) | prereq
	@echo "building 48k rom 0x0800-0xBFFF, sega-boot-rom in cpu rom socket"
	zcc +z80 --list -m -vn -SO3 -compiler=sdcc -startup=1 $(SOURCE) -o gamerom -create-app $(CFLAGS) -Cz"--rombase=0x0800 --romsize=47104"
	@mv gamerom* build/ 2>/dev/null || true
	@mv *.lis build/ 2>/dev/null || true
	@printf 'ROM usage: ' && stat -f '%z' build/gamerom_CODE.bin
	@printf 'RAM usage: ' && echo "$$(stat -f '%z' build/gamerom_BSS.bin) + $$(stat -f '%z' build/gamerom_DATA.bin)" | bc
	@grep -q "/crt_page_zero_z80.inc:" build/gamerom.lis || (echo "ERROR crt_page_zero missing. please edit z80_crt_o.asm.m4" && exit 1)
	@rm -f $@
	cat build/bootrom.bin >> $@
	cat build/gamerom.rom >> $@
	truncate -s 48K $@

build/bootrom.bin: bootrom.asm | prereq
	z80asm bootrom.asm -o $@
	truncate -s 2K $@

# --- ROM generation

roms: build/gamerom.bin | prereq
	@mkdir -p roms
	@mkdir -p build/rom
	@cp build/bootrom.bin roms/1873.cpu-u25
	@split -d -b 2048 build/gamerom.bin build/rom/
	@cp build/rom/01 roms/1848.prom-u1
	@cp build/rom/02 roms/1849.prom-u2
	@cp build/rom/03 roms/1850.prom-u3
	@cp build/rom/04 roms/1851.prom-u4
	@cp build/rom/05 roms/1852.prom-u5
	@cp build/rom/06 roms/1853.prom-u6
	@cp build/rom/07 roms/1854.prom-u7
	@cp build/rom/08 roms/1855.prom-u8
	@cp build/rom/09 roms/1856.prom-u9
	@cp build/rom/10 roms/1857.prom-u10
	@cp build/rom/11 roms/1858.prom-u11
	@cp build/rom/12 roms/1859.prom-u12
	@cp build/rom/13 roms/1860.prom-u13
	@cp build/rom/14 roms/1861.prom-u14
	@cp build/rom/15 roms/1862.prom-u15
	@cp build/rom/16 roms/1863.prom-u16
	@cp build/rom/17 roms/1864.prom-u17
	@cp build/rom/18 roms/1865.prom-u18
	@cp build/rom/19 roms/1866.prom-u19
	@cp build/rom/20 roms/1867.prom-u20
	@cp build/rom/21 roms/1868.prom-u21
	@cp build/rom/22 roms/1869.prom-u22
	@cp build/rom/23 roms/1870.prom-u23

mame: roms
	# add any missing files required by mame (never overwrite what we just built)
	@unzip -n -j ../attack-vektor/startrek.zip -d roms
	# and a couple of extra files, incase we're running hacked mamedev.
	@truncate -s 32 roms/187x.speech-u3
	@truncate -s 32 roms/187x.speech-u4
	zip -qj roms/startrek.zip roms/* -x "*.zip"
	$(MAME) -rp ./roms startrek

# --- art

# sine table PROM used to simulate the vector generator's DDA and compensate for its
# integer rounding (see tools/svg2sega.py --hw-scale/--sintable). roms/ gets wiped by
# `make clean` and isn't repopulated until the mame: recipe runs (after art/gamerom.bin
# already built), so read from startrek/ which persists across clean instead.
SINTABLE := startrek/s-c.xyt-u39

art: $(HDRS) | prereq

$(ART_BUILD_DIR)/%.h: art/%.svg
	../tools/svg2sega.py --scale 0.6 --rotate 90 --speed 0 --rdp 1.8 --close --sintable $(SINTABLE) $< > $@

# logo/bg_planet are drawn at a non-default symbol scale (see LOGO_SCALE/BG_PLANET_SCALE in
# main.c); the DDA compensation only lands closed shapes on-pixel at the scale they're
# actually drawn at, so these override the generic 1x (0x80) default.
$(ART_BUILD_DIR)/logo.h: art/logo.svg
	../tools/svg2sega.py --scale 0.6 --rotate 90 --speed 0 --rdp 1.8 --close --sintable $(SINTABLE) --hw-scale 0xff $< > $@

$(ART_BUILD_DIR)/logo-planet.h: art/logo-planet.svg
	../tools/svg2sega.py --scale 0.6 --rotate 90 --speed 0 --rdp 1.8 --close --sintable $(SINTABLE) --hw-scale 0xa0 $< > $@

$(ART_BUILD_DIR)/%.h: art/font/%.svg
	../tools/svg2sega.py --scale 0.4 --rotate 90 --speed 0 --origin-top-left --close --sintable $(SINTABLE) $< > $@

# --- Hardware Programming

PROGRAMMER = python3 ~/Downloads/EPROM_EMU_NG_2.0rc9.py
UART = /dev/cu.usbserial-B00050RO

flash:
	 ${PROGRAMMER} -mem 27512 -spi y -auto y -start 0 build/gamerom.bin ${UART}
