#export PATH=${PATH}:/Users/jeff/Desktop/z88dk/bin
#export ZCCCFG=/Users/jeff/Desktop/z88dk/lib/config
# see https://github-wiki-see.page/m/z88dk/z88dk/wiki/NewLib--Platform--Embedded

# ~/Downloads/dasmxx-master/src/dasmz80 dasmxx/sega.txt > dasmxx/gamerom.asm
# minipro -p M2716@DIP24 -w /Users/jeff/Downloads/bootrom.bin  --vpp=25

# patch sdcc for crt_page_zero
# see https://www.z88dk.org/forum/viewtopic.php?t=12010

all: gamerom

prereq:
	@mkdir -p build
	@mkdir -p roms

clean:
	$(RM) -rf build
	$(RM) -rf roms
	$(RM) art/*.h
	$(RM) art/font/*.h

# development is made easier by loading bootrom into 1873.cpu-u25
# then romulator can be quickly loaded to a single large rom on the ROM board

CFLAGS = -DENABLE_BOOTROM
CFLAGS += -DMAME_BUILD

#MAME = mame
MAME = /Users/jmathews/Downloads/mame-master/mamed

gamerom: prereq bootrom art_headers
	@echo "building 32k rom at 0x800 for use with sega-boot-rom in cpu rom socket"
	zcc +z80 --list -m -vn -SO3 -compiler=sdcc -startup=1 font.c math.c main.c usb.c -o $@ -create-app $(CFLAGS) -Cz"--rombase=0x0800 --romsize=30720"
	@mv $@* build/ 2>/dev/null || true
	@mv *.lis build/ 2>/dev/null || true
	@printf 'ROM usage: ' && stat -f '%z' build/$@_CODE.bin
	@printf 'RAM usage: ' && echo "$$(stat -f '%z' build/$@_BSS.bin) + $$(stat -f '%z' build/$@_DATA.bin)" | bc
	@grep -q "/crt_page_zero_z80.inc:" build/$@.lis || (echo "ERROR crt_page_zero missing. please edit z80_crt_o.asm.m4" && exit 1)
	@rm -f build/$@.bin
	cat build/bootrom.bin >> build/$@.bin
	cat build/$@.rom >> build/$@.bin
	truncate -s 32K build/$@.bin

bootrom: prereq
	z80asm bootrom.asm -o build/$@.bin
	truncate -s 2K build/$@.bin

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
	unzip startrek.zip -d startrek
	rsync -vt roms/* startrek
	zip roms.zip startrek/*
	rm -r startrek
	mv roms.zip build/startrek.zip
	$(MAME) -rp ./build startrek


SVGS_ROOT := $(wildcard art/*.svg)
SVGS_FONT := $(wildcard art/font/*.svg)
HDRS_ROOT := $(SVGS_ROOT:.svg=.h)
HDRS_FONT := $(SVGS_FONT:.svg=.h)
HDRS := $(HDRS_ROOT) $(HDRS_FONT)

.PHONY: art_headers
art_headers: $(HDRS)

$(HDRS_ROOT): art/%.h: art/%.svg
	../tools/svg2sega.py --speed 0 --merge 0 --rdp 1.0 --close $< > $@

$(HDRS_FONT): art/font/%.h: art/font/%.svg
	../tools/svg2sega.py --scale 0.4 --rotate 90 --speed 0 --close $< > $@


# ROM Emulator https://github.com/Kris-Sekula/EPROM-EMU-NG/
# python3 -m pip install serial
# python3 -m pip install PySimpleGUI
# brew install python-tk

PROGRAMMER = python3 ~/Downloads/EPROM_EMU_NG_2.0rc9.py
UART = /dev/cu.usbserial-B00050RO

flash:
	 ${PROGRAMMER} -mem 27256 -spi y -auto y -start 0 build/gamerom.bin ${UART}

