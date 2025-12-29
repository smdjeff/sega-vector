# sega-xy-diagnostic

## New Sega G80 diagnostics ROM

| | 2024 Diagnostic ROM  | David Shuman's ROM |
| ------------- | ------------- | ------------- |
| Test Coverage| RAM, ROMs Speech, sounds  | RAM and XY display |
| Test Results| Through sound boards | LED and XY display |
| Requirements | CPU RAM | CPU RAM, XY clock, XY RAM |
| Optional Requirements | Sound board, speech board, ROM U10,11,12 | LED, NMI, buttons |


# Download the ROM 

Load into the 2716 ROM programmer of your choice and insert into the CPU socket.

[2716 TEST ROM](build/testrom.rom)

# Usage, what to expect

* Power up boardset with ROM installed into CPU board
* Speech "start"
  
* One beep: sound test starts
  * Start button: exit
  * Fire button: triggers next sound
  * Thrust button: triggers previous sound
  * Photon button: resets the USB board
    
* Two beeps: ram test starts
  * USB Ram 1 speech "one", "red alert" on failure...
  * USB Ram 2 speech "two"
  * Vector Ram 1 speech "three"
  * Vector Ram 2 speech "four"
    
* Three beeps rom tests starts
  * 1848.prom-u1 speech "one", "red alert" on failure...
  * ...
  * 1857.prom-u10 speech "one" "zero"
  * ...
  * 1870.prom-u23 speech "two" "three"
    
* Four beeps: speech test starts

## Sound Test
1. Impulse
1. Impulse End
1. Warp
1. Warp End
1. Red Alert
1. Red Alert End
1. Phaser
1. Photon
1. Targeting
1. Deny
1. Shield Hit
1. Enterprise Hit
1. Enterprise Explosion
1. Klingon Explosion
1. Dock
1. Starbase Hit
1. Starbase Red
1. Warp Suck
1. Warp Suck End
1. Saucer Exit
1. Saucer Exit End
1. Starbase Explosion
1. Small Bonus
1. Large Bonus
1. Starbase Intro
1. Klingon Intro
1. Enterprise Intro
1. Player Change
1. Klingon Fire
1. High Score Music
1. Coin Drop Music
1. Nomad Motion
1. Nomad Motion End
1. Nomad Stopped
1. Nomad Stopped End

## Speech Test
1. Command The Enterprise
1. Play Star Trek
1. Welcome Aboard, Captain (Spock)
1. Congratulations
1. High Score
1. Press Player One
1. Or Player Two
1. Start
1. Be The Captain Of The Starship Enterprise (Scotty)
1. Damage Repaired, Sir (Scotty)
1. Sector Secured (Chekov)
1. Entering Sector (Spock)
1. Zero
1. One
1. Two
1. Three
1. Four
1. Five
1. Six
1. Seven
1. Eight
1. Nine
1. Point
1. Point High Pitch
1. Red Alert

   
# Building the ROM yourself
```
export PATH=${PATH}:/Users/jmathews/Desktop/z88dk/bin
export ZCCCFG=/Users/jmathews/Desktop/z88dk/lib/config
make
```

# Stand alone sound board testing

The Sega Universal Serial Board consists of:
* 6 Digital to Analog Converters
* 6 three channel analog multiplexers
* 1 noise fgeneration chip, the 5837
* 2 1K x 4bit RAMs (used for scratch memory)
* 2 6116 2K x 8bit RAMs (used as program memory)

The two 6116 RAMs at U50 and U51 are actually program memory for the MCS-48 CPU. They're loaded dynamically from the main Z80 CPU. There are actually three images. The low 2K bytes are fixed and loaded once at boot. Then, the high 2K bytes are loaded and reloaded quite often to switch between music mode and sound effects mode.

Interestingly, you can actually rip the 6116s off the board and load them with 2716s preprogrammed with those images. This allows testing of the soundboard independently of the CPU and ROM boards!
