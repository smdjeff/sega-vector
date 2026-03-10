# sega-g80-vector Star Trek 2

# A new game written for a vintage Sega 1982 vector arcade machine

# Installation
* The build folder contains a zip file with all the ROMs necessary to run the game
  
## Running in MAME
* Simply point MAME to the build/zip and run
* _Note: additional speech data may be needed._ Modify mame/segag80v.cpp and rebuild
  ```
  ROM_REGION( 0x4000, "speech:data", 0 )
  ROM_LOAD( "1871.speech-u6",  0x0000, 0x1000, CRC(0) SHA1(0) )
  ROM_LOAD( "1872.speech-u5",  0x1000, 0x1000, CRC(0) SHA1(0) )
  ROM_LOAD( "187x.speech-u4",  0x2000, 0x1000, CRC(0) SHA1(0) )
  ROM_LOAD( "187x.speech-u3",  0x3000, 0x1000, CRC(0) SHA1(0) )
  ```

## Running on Real Sega G80 Hardware
1. Burn and load 1670.speech-u7 into Speech Board's 8035 CPU  _Note: occasionally named 1607_ 
2. Burn and load 1871.speech-u6 through 187x.speech-u3 into Speech Board's Data
4. Burn and load 1873.cpu-u25 into Main board's Z80 CPU
5. Burn and load 1848.prom-u1 through 1870.prom-u23 into ROM board  _Note: Only those with a recent date are actually required_

## Enjoy!
Follow the development progress on [YouTube shorts](https://youtube.com/playlist?list=PL5WwuS3ViybqfLWkKmgaT5_N2kVawZYZk)
