#export PATH=${PATH}:/Users/jeff/Desktop/z88dk/bin
#export ZCCCFG=/Users/jeff/Desktop/z88dk/lib/config

# see https://github-wiki-see.page/m/z88dk/z88dk/wiki/NewLib--Platform--Embedded



all: testrom

prereq:
	@mkdir -p build

clean:
	$(RM) -rf build


# test rom controlled by control panel buttons
testrom: prereq
	@echo "building rom to test sounds. requires working ROMs at U10,U11,U12"
	zcc +z80 -vn -O3 -startup=1 -clib=new main.c -o $@ -create-app -Cz"--romsize=2048"
	hexdump $@.rom
	@mv $@* build/ 2>/dev/null || true

# ROM Emulator https://github.com/Kris-Sekula/EPROM-EMU-NG/
# python3 -m pip install serial
# python3 -m pip install PySimpleGUI
# brew install python-tk

PROGRAMMER = python3 ~/Downloads/EPROM_EMU_NG_2.0rc9.py
UART = /dev/cu.usbserial-14110

flash:
	 ${PROGRAMMER} -mem 2716 -spi y -auto y -start 0 build/testrom.rom ${UART}
