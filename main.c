
////////////////////////////////////
// dependencies:
// https://github.com/z88dk/z88dk/releases
// export PATH=${PATH}:/Users/jmathews/Desktop/z88dk/bin
// export ZCCCFG=/Users/jmathews/Desktop/z88dk/lib/config

////////////////////////////////////
#include "sega.h"



static volatile uint8_t nmi_counter = 0;
static volatile uint16_t system_tick = 0;
static volatile uint8_t _coin_counter = 0;
static uint8_t score = 0;
static uint8_t high_score[3] = { 20, 15, 1 };
static char high_name[3][4] = { "jef", "sno", "amy" };
static game_state_t game_state = game_state_boot;
static uint8_t sound_track = 0;
static uint8_t missle_max = 1;

#define CUBE_X0 (MIN_X+40)
#define CUBE_X1 (MAX_X-70)
#define CUBE_Y0 (CENTER_Y-300)
#define CUBE_Y1 (CENTER_Y)
#define CUBE_Y2 (CENTER_Y+300)


// NMI int (the cpu board button was pushed)
void z80_nmi(void) __critical __interrupt {
   nmi_counter++;
}


static void timer_interrupt_4Hz(void);

// IRQ signal comes from multiple sources:
//    * XINT signal, which is a combination of:
//        - COINA impulse, clocks an LS74, cleared by INTCL signal
//        - COINB impulse, clocks an LS74, cleared by INTCL signal
//        - SERVICE impulse
//    * /EDGINT signal from vector board, clocks an LS74, cleared by INTCL signal
//        - signal comes from 15468480 crystal, divided by 3, and then by 0x1f788
void z80_rst_38h (void) __critical __interrupt(0) {

   // this is a 40Hz, 25ms timer
   system_tick++;

   static uint8_t div = 0;
   if ( div >= 5 ) {
      div = 0;
      // this is a 8Hz, 125ms timer
      timer_interrupt_4Hz();
   }
   div++;

   static uint8_t debounce = 0;
   static uint8_t button_last = 0xff;
   volatile uint8_t button = PORT_370; // force INTCL
   uint8_t button_edge = (button_last ^ button) & ~button; // falling edge only
   if ( debounce == 0 ) {
      if (button_edge & IO_COIN0_N) {
         _coin_counter++;
         SOUND_COMMAND = COIN_DROP;
         debounce = 20;
      }
      if (button_edge & IO_SERVICE_N) {
         if ( game_state != game_state_diagnostics_io ) {
            game_state = game_state_diagnostics_io_init;
         } else {
            game_state = game_state_diagnostics_grid_init;
         }
        debounce = 20;
      }
   } else {
      debounce--;
   }
   button_last = button;
}




static void delay(uint16_t ms) {
   while ( ms-- ) {
      for (uint16_t i=0; i<27; i++) {   // 1ms at 3.86712 MHz (when XY board is clocking)
      //for (uint16_t i=0; i<28; i++) {   // 1ms at 4.0 MHz (when CPU board is self clocking)
         __asm__( "nop" );
      }
   }
}


static void sound_init(void) {
   extern const uint8_t usbrom_bin[];
   // copy main board ROMs into sound board's RAM 
   // so the 8035 can execute it
   SOUND_COMMAND = 0xFF; // assert RAM LOAD latch
   memcpy( (uint8_t*)USB_RAM, usbrom_bin, (1024*4) );
   SOUND_COMMAND = 0x7F; 
}


static void say(uint8_t i) {
   static uint16_t last_tick = 0;
   while ( system_tick - last_tick < 750/25 ) {
      // wait for speech to finish, sending too fast causes glitches
      // but unclear how to receive interrupt on complete of phrase
      // so just busy wait
      //SPEECH_COMMAND = NO_PHRASE;
      __asm__( "nop" );
   }
   SPEECH_COMMAND = i;
   SPEECH_COMMAND = i | 0x80;
   last_tick = system_tick;
}

static void digits3( uint8_t *d0, uint8_t *d1, uint8_t *d2, uint8_t r ) {
   *d0 = '0' + divideBy100( &r ); 
   *d1 = '0' + divideBy10( &r );
   *d2 = '0' + r;
}


#pragma disable_warning 110 // flow changed by optimizer
#pragma disable_warning 126 // unreachable code

static inline void stretch3(uint8_t *p, uint16_t i) {
   p[sizeof(vector_t)*0] = MIN(255,i); i-=MIN(255,i);
   p[sizeof(vector_t)*1] = MIN(255,i); i-=MIN(255,i);
   p[sizeof(vector_t)*2] = MIN(255,i);
}

static inline void stretch5(uint8_t *p, uint16_t i) {
   p[sizeof(vector_t)*0] = MIN(255,i); i-=MIN(255,i);
   p[sizeof(vector_t)*1] = MIN(255,i); i-=MIN(255,i);
   p[sizeof(vector_t)*2] = MIN(255,i); i-=MIN(255,i);
   p[sizeof(vector_t)*3] = MIN(255,i); i-=MIN(255,i);
   p[sizeof(vector_t)*4] = MIN(255,i);
}



   const uint8_t vector[] = {
       #define V_BLANK (0)
       SEGA_COLOR_GRAY|SEGA_LAST,    0, LE(SEGA_ANGLE(0)),

       #define V_LINE  (V_BLANK+1)
       SEGA_CLEAR,                   0x25, LE(SEGA_ANGLE(130)),
       SEGA_COLOR_YELLOW|SEGA_LAST,  0x1C, LE(SEGA_ANGLE(270)),

       #define V_TANK (V_LINE+2)
       SEGA_CLEAR,                  SIZE(13),  LE(SEGA_ANGLE(320)),
       SEGA_COLOR_GREEN,            SIZE(4),   LE(SEGA_ANGLE(90)),
       SEGA_COLOR_GREEN,            SIZE(1),   LE(SEGA_ANGLE(180)),
       SEGA_COLOR_GREEN,            SIZE(8),   LE(SEGA_ANGLE(90)),
       SEGA_COLOR_GREEN,            SIZE(1),   LE(SEGA_ANGLE(0)),
       SEGA_COLOR_GREEN,            SIZE(4),   LE(SEGA_ANGLE(90)),
       SEGA_COLOR_GREEN,            SIZE(20),  LE(SEGA_ANGLE(180)),
       SEGA_COLOR_GREEN,            SIZE(4),   LE(SEGA_ANGLE(270)),
       SEGA_COLOR_GREEN,            SIZE(1),   LE(SEGA_ANGLE(0)),
       SEGA_COLOR_GREEN,            SIZE(8),   LE(SEGA_ANGLE(270)),
       SEGA_COLOR_GREEN,            SIZE(1),   LE(SEGA_ANGLE(180)),
       SEGA_COLOR_GREEN,            SIZE(4),   LE(SEGA_ANGLE(270)),
       SEGA_COLOR_GREEN|SEGA_LAST,  SIZE(20),  LE(SEGA_ANGLE(0)),

       #define V_TREAD  (V_TANK+13)
       SEGA_CLEAR,                     SIZE(13),  LE(SEGA_ANGLE(320)),
       SEGA_CLEAR,                     SIZE(3),   LE(SEGA_ANGLE(180)),   // animation sequence: 1,2,3
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(90)),
       SEGA_CLEAR,                     SIZE(8),   LE(SEGA_ANGLE(90)),
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(90)),
       SEGA_CLEAR,                     SIZE(3),   LE(SEGA_ANGLE(180)),
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(270)),
       SEGA_CLEAR,                     SIZE(8),   LE(SEGA_ANGLE(270)),
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(270)),
       SEGA_CLEAR,                     SIZE(3),   LE(SEGA_ANGLE(180)),
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(90)), 
       SEGA_CLEAR,                     SIZE(8),   LE(SEGA_ANGLE(90)), 
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(90)),
       SEGA_CLEAR,                     SIZE(3),   LE(SEGA_ANGLE(180)),
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(270)),
       SEGA_CLEAR,                     SIZE(8),   LE(SEGA_ANGLE(270)),
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(270)),
       SEGA_CLEAR,                     SIZE(3),   LE(SEGA_ANGLE(180)),
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(90)),
       SEGA_CLEAR,                     SIZE(8),   LE(SEGA_ANGLE(90)),
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(90)),
       SEGA_CLEAR,                     SIZE(3),   LE(SEGA_ANGLE(180)),
       SEGA_COLOR_GREEN1,              SIZE(4),   LE(SEGA_ANGLE(270)),
       SEGA_CLEAR,                     SIZE(8),   LE(SEGA_ANGLE(270)), 
       SEGA_COLOR_GREEN1|SEGA_LAST,    SIZE(4),   LE(SEGA_ANGLE(270)), 


     #define V_TURRET (V_TREAD+25)
     SEGA_CLEAR,                       SIZE(7.5), LE(SEGA_ANGLE(45)),  // 0
     SEGA_COLOR_GREEN,                 SIZE(11),  LE(SEGA_ANGLE(180)), // 1
     SEGA_COLOR_GREEN,                 SIZE(11),  LE(SEGA_ANGLE(270)), // 2
     SEGA_COLOR_GREEN,                 SIZE(11),  LE(SEGA_ANGLE(0)),   // 3
     SEGA_COLOR_GREEN,                 SIZE(11),  LE(SEGA_ANGLE(90)),  // 4
     SEGA_COLOR_GREEN1,                SIZE(2),   LE(SEGA_ANGLE(225)), // 5
     SEGA_COLOR_GREEN,                 SIZE(8),   LE(SEGA_ANGLE(180)), // 6
     SEGA_CLEAR,                       SIZE(2),   LE(SEGA_ANGLE(135)), // 7
     SEGA_COLOR_GREEN1,                SIZE(2),   LE(SEGA_ANGLE(315)), // 8
     SEGA_COLOR_GREEN,                 SIZE(8),   LE(SEGA_ANGLE(270)), // 9
     SEGA_CLEAR,                       SIZE(2),   LE(SEGA_ANGLE(225)), // 10
     SEGA_COLOR_GREEN1,                SIZE(2),   LE(SEGA_ANGLE(45)),  // 11
     SEGA_COLOR_GREEN,                 SIZE(8),   LE(SEGA_ANGLE(0)),   // 12
     SEGA_CLEAR,                       SIZE(2),   LE(SEGA_ANGLE(315)), // 13
     SEGA_COLOR_GREEN1,                SIZE(2),   LE(SEGA_ANGLE(135)), // 14
     SEGA_COLOR_GREEN|SEGA_LAST,       SIZE(8),   LE(SEGA_ANGLE(90)),  // 15

     #define V_BARREL (V_TURRET+16)
     SEGA_CLEAR,                       SIZE(5),   LE(SEGA_ANGLE(0)),   // 0
     SEGA_CLEAR,                       SIZE(1),   LE(SEGA_ANGLE(270)), // 1
     SEGA_COLOR_GREEN1,                SIZE(8),   LE(SEGA_ANGLE(0)),   // 2
     SEGA_COLOR_GREEN,                 SIZE(1),   LE(SEGA_ANGLE(270)), // 3
     SEGA_COLOR_GREEN,                 SIZE(1),   LE(SEGA_ANGLE(0)),   // 4
     SEGA_COLOR_GREEN,                 SIZE(4),   LE(SEGA_ANGLE(90)),  // 5
     SEGA_COLOR_GREEN,                 SIZE(1),   LE(SEGA_ANGLE(180)), // 6
     SEGA_COLOR_GREEN,                 SIZE(1),   LE(SEGA_ANGLE(270)), // 7
     SEGA_COLOR_GREEN1|SEGA_LAST,      SIZE(8),   LE(SEGA_ANGLE(180)), // 8
     SEGA_COLOR_GREEN,                 SIZE(2.8), LE(SEGA_ANGLE(315)), // 9
     SEGA_CLEAR,                       SIZE(2.8), LE(SEGA_ANGLE(0)),   // 10
     SEGA_COLOR_GREEN,                 SIZE(2.8), LE(SEGA_ANGLE(135)), // 11
     SEGA_CLEAR,                       SIZE(2.8), LE(SEGA_ANGLE(0)),   // 12
     SEGA_COLOR_GREEN|SEGA_LAST,       SIZE(2.8), LE(SEGA_ANGLE(315)), // 13
     #define V_BARREL_LEN 14

     #define V_FLAME (V_BARREL+V_BARREL_LEN)
     SEGA_CLEAR,                       SIZE(14),  LE(SEGA_ANGLE(347)),
     SEGA_COLOR_WHITE,                 SIZE(3),   LE(SEGA_ANGLE(315)),
     SEGA_COLOR_WHITE,                 SIZE(2),   LE(SEGA_ANGLE(90)),
     SEGA_COLOR_WHITE,                 SIZE(2),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_WHITE,                 SIZE(2),   LE(SEGA_ANGLE(135)),
     SEGA_COLOR_WHITE,                 SIZE(4),   LE(SEGA_ANGLE(22)),
     SEGA_COLOR_WHITE,                 SIZE(4),   LE(SEGA_ANGLE(158)),
     SEGA_COLOR_WHITE,                 SIZE(2),   LE(SEGA_ANGLE(45)),
     SEGA_COLOR_WHITE,                 SIZE(2),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_WHITE,                 SIZE(2),   LE(SEGA_ANGLE(90)),
     SEGA_COLOR_WHITE|SEGA_LAST,       SIZE(3),   LE(SEGA_ANGLE(225)),

     #define V_MISSILE (V_FLAME+11)
     SEGA_CLEAR,                       SIZE(2),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_YELLOW,                SIZE(1.2), LE(SEGA_ANGLE(158)),
     SEGA_COLOR_YELLOW,                SIZE(2),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_YELLOW,                SIZE(1),   LE(SEGA_ANGLE(135)),
     SEGA_COLOR_YELLOW,                SIZE(2.2), LE(SEGA_ANGLE(270)),
     SEGA_COLOR_YELLOW,                SIZE(1),   LE(SEGA_ANGLE(45)),
     SEGA_COLOR_YELLOW,                SIZE(2),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_YELLOW|SEGA_LAST,      SIZE(1.2), LE(SEGA_ANGLE(22)),

     #define V_BONUS (V_MISSILE+8)
     SEGA_CLEAR,                       SIZE(2),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_GREEN,                 SIZE(1.2), LE(SEGA_ANGLE(158)),
     SEGA_COLOR_GREEN,                 SIZE(2),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_GREEN,                 SIZE(1),   LE(SEGA_ANGLE(135)),
     SEGA_COLOR_GREEN,                 SIZE(2.2), LE(SEGA_ANGLE(270)),
     SEGA_COLOR_GREEN,                 SIZE(1),   LE(SEGA_ANGLE(45)),
     SEGA_COLOR_GREEN,                 SIZE(2),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_GREEN,                 SIZE(1.2), LE(SEGA_ANGLE(22)),
     SEGA_CLEAR,                       SIZE(3.5), LE(SEGA_ANGLE(135)),
     0x54, 0x18, 0x00, 0x02,
     0x55, 0x1c, 0x00, 0x01,
     0x55, 0x18, 0x01, 0x00,
     0x55, 0x1c, 0x01, 0x03,
     0x54, 0x0c, 0x02, 0x02,
     0xd5, 0x1c, 0x01, 0x01,

     #define V_BLADE (V_BONUS+15)
     SEGA_COLOR_YELLOW,                  SIZE(1.3), LE(SEGA_ANGLE(55)),
     SEGA_COLOR_YELLOW,                  SIZE(9),   LE(SEGA_ANGLE(90)),
     SEGA_COLOR_YELLOW,                  SIZE(1),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_YELLOW,                  SIZE(9),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_YELLOW,                  SIZE(2),   LE(SEGA_ANGLE(293)),
     SEGA_COLOR_YELLOW,                  SIZE(9),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_YELLOW,                  SIZE(1),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_YELLOW,                  SIZE(9),   LE(SEGA_ANGLE(90)),
     SEGA_COLOR_YELLOW,                  SIZE(1),   LE(SEGA_ANGLE(68)),
     SEGA_COLOR_YELLOW,                  SIZE(1),   LE(SEGA_ANGLE(158)),
     SEGA_COLOR_YELLOW,                  SIZE(9),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_YELLOW,                  SIZE(1),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_YELLOW,                  SIZE(9),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_YELLOW,                  SIZE(2),   LE(SEGA_ANGLE(22)),
     SEGA_COLOR_YELLOW,                  SIZE(9),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_YELLOW,                  SIZE(1),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_YELLOW,                  SIZE(9),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_YELLOW|SEGA_LAST,        SIZE(1),   LE(SEGA_ANGLE(158)),

     #define V_CHOPPER (V_BLADE+18)
     SEGA_CLEAR,                       SIZE(2.6), LE(SEGA_ANGLE(330)),
     SEGA_COLOR_ORANGE,                  SIZE(2),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_ORANGE,                  SIZE(2),   LE(SEGA_ANGLE(90)),
     SEGA_COLOR_ORANGE,                  SIZE(2),   LE(SEGA_ANGLE(158)),
     SEGA_COLOR_ORANGE,                  SIZE(5),   LE(SEGA_ANGLE(90)),
     SEGA_COLOR_ORANGE,                  SIZE(3),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_ORANGE,                  SIZE(6),   LE(SEGA_ANGLE(90)),
     SEGA_COLOR_ORANGE,                  SIZE(4),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_ORANGE,                  SIZE(1),   LE(SEGA_ANGLE(90)),
     SEGA_COLOR_ORANGE,                  SIZE(7),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_ORANGE,                  SIZE(1),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_ORANGE,                  SIZE(4),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_ORANGE,                  SIZE(6),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_ORANGE,                  SIZE(3),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_ORANGE,                  SIZE(5),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_ORANGE,                  SIZE(2),   LE(SEGA_ANGLE(203)),
     SEGA_COLOR_ORANGE,                  SIZE(2),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_ORANGE,                  SIZE(6.7), LE(SEGA_ANGLE(0)),
     SEGA_COLOR_ORANGE,                  SIZE(2),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_ORANGE,                  SIZE(4),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_ORANGE,                  SIZE(3),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_ORANGE,                  SIZE(2),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_ORANGE,                  SIZE(1),   LE(SEGA_ANGLE(270)),
     SEGA_COLOR_ORANGE,                  SIZE(1),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_ORANGE,                  SIZE(1),   LE(SEGA_ANGLE(90)),
     SEGA_COLOR_ORANGE,                  SIZE(2),   LE(SEGA_ANGLE(0)),
     SEGA_COLOR_ORANGE,                  SIZE(3),   LE(SEGA_ANGLE(90)),
     SEGA_COLOR_ORANGE,                  SIZE(4),   LE(SEGA_ANGLE(180)),
     SEGA_COLOR_ORANGE|SEGA_LAST,        SIZE(2),   LE(SEGA_ANGLE(90)),

     #define V_CUBE0 (V_CHOPPER+29)
     SEGA_CLEAR,                       SIZE(5.6), LE(SEGA_ANGLE(315)), // 0  beam wait
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(45)),  // 1  angle 
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(90)),  // 2  rear
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(180)), // 3  rear
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(225)), // 4  angle 180
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(270)), // 5  front
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(0)),   // 6  front
     SEGA_COLOR_RED,                   SIZE(11.2), LE(SEGA_ANGLE(135)), // 7  front
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(0)),   // 8  front
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(45)),  // 9  angle
     SEGA_CLEAR,                       SIZE(8),   LE(SEGA_ANGLE(270)), // 10  retrace
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(180)), // 11 rear
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(225)), // 12 angle 180
     SEGA_CLEAR,                       SIZE(8),   LE(SEGA_ANGLE(0)),   // 13 retrace
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(90)),  // 14 front
     SEGA_CLEAR,                       SIZE(2),   LE(SEGA_ANGLE(45)),  // 15 angle retrace
     SEGA_CLEAR,                       SIZE(8),   LE(SEGA_ANGLE(180)), // 16 retrace
     SEGA_COLOR_CYAN|SEGA_LAST,        SIZE(8),   LE(SEGA_ANGLE(270)), // 17 rear

     #define V_CUBE1 (V_CUBE0+18)
     SEGA_CLEAR,                       SIZE(5.6), LE(SEGA_ANGLE(315)), // 0  beam wait
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(45)),  // 1  angle 
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(90)),  // 2  rear
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(180)), // 3  rear
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(225)), // 4  angle 180
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(270)), // 5  front
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(0)),   // 6  front
     SEGA_COLOR_GREEN,                 SIZE(11.2), LE(SEGA_ANGLE(135)), // 7  front
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(0)),   // 8  front
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(45)),  // 9  angle
     SEGA_CLEAR,                       SIZE(8),   LE(SEGA_ANGLE(270)), // 10  retrace
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(180)), // 11 rear
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(225)), // 12 angle 180
     SEGA_CLEAR,                       SIZE(8),   LE(SEGA_ANGLE(0)),   // 13 retrace
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(90)),  // 14 front
     SEGA_CLEAR,                       SIZE(2),   LE(SEGA_ANGLE(45)),  // 15 angle retrace
     SEGA_CLEAR,                       SIZE(8),   LE(SEGA_ANGLE(180)), // 16 retrace
     SEGA_COLOR_CYAN|SEGA_LAST,        SIZE(8),   LE(SEGA_ANGLE(270)), // 17 rear

     #define V_CUBE2 (V_CUBE1+18)
     SEGA_CLEAR,                       SIZE(5.6), LE(SEGA_ANGLE(315)), // 0  beam wait
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(45)),  // 1  angle 
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(90)),  // 2  rear
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(180)), // 3  rear
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(225)), // 4  angle 180
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(270)), // 5  front
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(0)),   // 6  front
     SEGA_COLOR_BLUE,                  SIZE(11.2), LE(SEGA_ANGLE(135)), // 7  front
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(0)),   // 8  front
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(45)),  // 9  angle
     SEGA_CLEAR,                       SIZE(8),   LE(SEGA_ANGLE(270)), // 10  retrace
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(180)), // 11 rear
     SEGA_COLOR_CYAN,                  SIZE(2),   LE(SEGA_ANGLE(225)), // 12 angle 180
     SEGA_CLEAR,                       SIZE(8),   LE(SEGA_ANGLE(0)),   // 13 retrace
     SEGA_COLOR_CYAN,                  SIZE(8),   LE(SEGA_ANGLE(90)),  // 14 front
     SEGA_CLEAR,                       SIZE(2),   LE(SEGA_ANGLE(45)),  // 15 angle retrace
     SEGA_CLEAR,                       SIZE(8),   LE(SEGA_ANGLE(180)), // 16 retrace
     SEGA_COLOR_CYAN|SEGA_LAST,        SIZE(8),   LE(SEGA_ANGLE(270)), // 17 rear

     #define V_STREET (V_CUBE2+18)
     SEGA_CLEAR,                       255,       LE(SEGA_ANGLE(0)),   // 0 up adjustable
     SEGA_CLEAR,                       255,       LE(SEGA_ANGLE(0)),   // 1 up adjustable
     SEGA_CLEAR,                       200,       LE(SEGA_ANGLE(0)),   // 2 up adjustable
     SEGA_CLEAR,                       128,       LE(SEGA_ANGLE(0)),   // 3 up fixed offset - middle of road

     SEGA_CLEAR,                       0,         LE(SEGA_ANGLE(270)), // 4 left adjustable
     SEGA_CLEAR,                       0,         LE(SEGA_ANGLE(270)), // 5 left adjustable
     SEGA_CLEAR,                       0,         LE(SEGA_ANGLE(270)), // 6 left adjustable
     SEGA_CLEAR,                       0,         LE(SEGA_ANGLE(270)), // 7 left adjustable
     SEGA_CLEAR,                       0,         LE(SEGA_ANGLE(270)), // 8 left adjustable
     SEGA_CLEAR,                       128,       LE(SEGA_ANGLE(270)), // 9 left fixed offset for middle of road

     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(90)),  // 10
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(90)),  // 11
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(90)),  // 12
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(90)),  // 13
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(90)),  // 14

     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(0)),   // 15
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(0)),   // 16
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(0)),   // 17
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(0)),   // 18

     SEGA_CLEAR,                       255,       LE(SEGA_ANGLE(90)),  // 19

     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(180)),  // 20
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(180)),  // 21
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(180)),  // 22
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(180)),  // 23
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(180)),  // 24

     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(270)),  // 25
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(270)),  // 26
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(270)),  // 27
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(270)),  // 28
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(270)),  // 29

     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(180)),  // 30
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(180)),  // 31
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(180)),  // 32
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(180)),  // 33

     SEGA_CLEAR,                       255,       LE(SEGA_ANGLE(270)),  // 34

     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(0)),  // 35
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(0)),  // 36
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(0)),  // 37
     SEGA_COLOR_CYAN,                 255,       LE(SEGA_ANGLE(0)),  // 38
     SEGA_COLOR_CYAN|SEGA_LAST,       255,       LE(SEGA_ANGLE(0)),  // 39


     #define HITBOX_SZ 80
     #define V_BOX (V_STREET+40)
     SEGA_CLEAR,                       (HITBOX_SZ/1.4),  LE(SEGA_ANGLE(225)),  // 0
     SEGA_COLOR_MAGENTA,               HITBOX_SZ,        LE(SEGA_ANGLE(0)),    // 1
     SEGA_COLOR_MAGENTA,               HITBOX_SZ,        LE(SEGA_ANGLE(90)),   // 2
     SEGA_COLOR_MAGENTA,               HITBOX_SZ,        LE(SEGA_ANGLE(180)),  // 3
     SEGA_COLOR_MAGENTA|SEGA_LAST,     HITBOX_SZ,        LE(SEGA_ANGLE(270)),  // 4

     #define V_EXPLODE0 (V_BOX+5)
     SEGA_CLEAR,                       SIZE(3.1), LE(SEGA_ANGLE(248)),   // 0
     SEGA_COLOR_RED,                   SIZE(3),   LE(SEGA_ANGLE(293)),   // 1
     SEGA_COLOR_RED,                   SIZE(3),   LE(SEGA_ANGLE(68)),    // 2
     SEGA_COLOR_YELLOW,                SIZE(2),   LE(SEGA_ANGLE(0)),     // 3
     SEGA_COLOR_YELLOW,                SIZE(2),   LE(SEGA_ANGLE(135)),   // 4
     SEGA_COLOR_RED,                   SIZE(4),   LE(SEGA_ANGLE(22)),    // 5
     SEGA_COLOR_RED,                   SIZE(4),   LE(SEGA_ANGLE(158)),   // 6
     SEGA_COLOR_YELLOW,                SIZE(2),   LE(SEGA_ANGLE(45)),    // 7
     SEGA_COLOR_YELLOW,                SIZE(2),   LE(SEGA_ANGLE(180)),   // 8
     SEGA_COLOR_RED,                   SIZE(3),   LE(SEGA_ANGLE(113)),   // 9
     SEGA_COLOR_RED,                   SIZE(3),   LE(SEGA_ANGLE(248)),   // 10
     SEGA_COLOR_YELLOW,                SIZE(2),   LE(SEGA_ANGLE(180)),   // 11
     SEGA_COLOR_YELLOW,                SIZE(2),   LE(SEGA_ANGLE(315)),   // 12
     SEGA_COLOR_RED,                   SIZE(4),   LE(SEGA_ANGLE(203)),   // 13
     SEGA_COLOR_RED,                   SIZE(4),   LE(SEGA_ANGLE(338)),   // 14
     SEGA_COLOR_YELLOW,                SIZE(2),   LE(SEGA_ANGLE(225)),   // 15
     SEGA_COLOR_YELLOW|SEGA_LAST,      SIZE(2),   LE(SEGA_ANGLE(0)),     // 16

     #define V_EXPLODE1 (V_EXPLODE0+17)
     SEGA_CLEAR,                       SIZE(1),   LE(SEGA_ANGLE(180)),   // 0
     SEGA_COLOR_WHITE,                 SIZE(2),   LE(SEGA_ANGLE(0)),     // 1
     SEGA_CLEAR,                       SIZE(1.5), LE(SEGA_ANGLE(135)),   // 2
     SEGA_COLOR_YELLOW|SEGA_LAST,      SIZE(2),   LE(SEGA_ANGLE(270)),   // 3

     #define V_SMOKE (V_EXPLODE1+4)
      0x2A, 0x42, 0x22, 0x03,
      0x2B, 0x0a, 0xcf, 0x03,
      0x2B, 0x0a, 0x33, 0x00,
      0x2B, 0x08, 0x8b, 0x00,
      0x2B, 0x05, 0xd5, 0x00,
      0x2A, 0x09, 0x49, 0x01,
      0x2B, 0x0e, 0x3e, 0x00,
      0x2B, 0x10, 0x77, 0x00,
      0x2B, 0x0f, 0xb6, 0x00,
      0x2B, 0x11, 0xf7, 0x00,
      0x2B, 0x10, 0x33, 0x01,
      0x2B, 0x0e, 0x82, 0x01,
      0x2B, 0x0c, 0xdd, 0x01,
      0x2A, 0x0a, 0x77, 0x00,
      0x2B, 0x0b, 0x27, 0x01,
      0x2B, 0x0b, 0x80, 0x01,
      0x2B, 0x0e, 0xf1, 0x01,
      0x2B, 0x0b, 0x44, 0x02,
      0x2A, 0x08, 0xcf, 0x02,
      0x2B, 0x12, 0x58, 0x01,
      0x2B, 0x16, 0xb3, 0x01,
      0x2B, 0x12, 0x30, 0x02,
      0x2B, 0x11, 0x8e, 0x02,
      0x2B, 0x14, 0xd2, 0x02,
      0x2B, 0x11, 0x44, 0x03,
      0x2A, 0x0d, 0xc1, 0x01,
      0x2B, 0x0a, 0x47, 0x02,
      0x2B, 0x0c, 0x93, 0x02,
      0x2B, 0x0c, 0xf1, 0x02,
      0x2B, 0x0b, 0x4c, 0x03,
      0x2B, 0x0c, 0xa7, 0x03,
      0x2A, 0x0c, 0x4f, 0x00,
      0x2B, 0x14, 0xdd, 0x02,
      0x2B, 0x12, 0x22, 0x03,
      0x2B, 0x15, 0x9c, 0x03,
      0x2B, 0x16, 0xee, 0x03,
      0x2B, 0x0d, 0x3b, 0x00,
      0xAB, 0x0d, 0x77, 0x00,
     #define V_SMOKE_LEN 38
     #define V_LAST (V_SMOKE+V_SMOKE_LEN)

    };


   const uint8_t symbol[] = {
      // 10 bytes each entry
      // flags      x             y             address     rotation             scale

      #define S_SCORE0    0
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BLANK)),     LE(0),               0x80,
      #define S_SCORE1    1
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BLANK)),     LE(0),               0x80,
      #define S_SCORE2    2
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BLANK)),     LE(0),               0x80,
      #define S_NAME0     3
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BLANK)),     LE(0),               0x40,
      #define S_NAME1     4
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BLANK)),     LE(0),               0x40,
      #define S_NAME2     5
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BLANK)),     LE(0),               0x40,
      #define S_STRING    6
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BLANK)),     LE(0),               0x40,

      #define S_TANK      7
      0, LE(1024), LE(1024), LE(V_ADDR(V_TANK)),  LE(SEGA_ANGLE(0)),   0x40,
      #define S_TREAD     8
      0, LE(1024), LE(1024), LE(V_ADDR(V_TREAD)), LE(SEGA_ANGLE(0)),   0x40,
      #define S_TURRET    9
      0, LE(1024), LE(1024), LE(V_ADDR(V_TURRET)), LE(SEGA_ANGLE(0)),   0x40,
      #define S_BARREL   10
      0, LE(1024), LE(1024), LE(V_ADDR(V_BARREL)), LE(SEGA_ANGLE(0)),   0x40,
      #define S_FLAME    11
      0,            LE(1024), LE(1024), LE(V_ADDR(V_FLAME)), LE(SEGA_ANGLE(0)),   0x40,
      #define S_MISSLE0   12
      0,            LE(1024), LE(1024), LE(V_ADDR(V_MISSILE)), LE(SEGA_ANGLE(0)),   0x40,
      #define S_MISSLE1   13
      0,            LE(1024), LE(1024), LE(V_ADDR(V_MISSILE)), LE(SEGA_ANGLE(0)),   0x40,
      #define S_MISSLE2   14
      0,            LE(1024), LE(1024), LE(V_ADDR(V_MISSILE)), LE(SEGA_ANGLE(0)),   0x40,

      #define S_BOX      15
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BOX)), LE(SEGA_ANGLE(0)),   0x80,

      #define S_CHOPPER  16
      0,            LE(1024), LE(MAX_Y-35), LE(V_ADDR(V_CHOPPER)), LE(0),     0x50,
      #define S_BLADE    17
      0,            LE(1024), LE(MAX_Y-35), LE(V_ADDR(V_BLADE)), LE(0),     0x50,

      #define S_CUBE0    18
      0, LE(CUBE_X0), LE(CUBE_Y0), LE(V_ADDR(V_CUBE0)), LE(0),     0xe0,
      #define S_CUBE1    19
      0, LE(CUBE_X1), LE(CUBE_Y1), LE(V_ADDR(V_CUBE1)), LE(0),     0xe0,
      #define S_CUBE2    20
      0, LE(CUBE_X0), LE(CUBE_Y2), LE(V_ADDR(V_CUBE2)), LE(0),    0xe0,

      #define S_BONUS    21
      0, LE(1024), LE(CUBE_Y2), LE(V_ADDR(V_BONUS)), LE(SEGA_ANGLE(0)),   0x30,

      #define S_STREET   22
      0,  LE(1024), LE(1024), LE(V_ADDR(V_STREET)), LE(0),    0x80,

      #define S_EXPLODE0 23
      0,             LE(1024), LE(1024), LE(V_ADDR(V_EXPLODE0)), LE(SEGA_ANGLE(0)),   0x40,
      #define S_EXPLODE1 24
      0,             LE(1024), LE(1024), LE(V_ADDR(V_EXPLODE0)), LE(SEGA_ANGLE(45)),   0x20,

      #define S_EXPLODE2 25
      0,             LE(1024), LE(1024), LE(V_ADDR(V_EXPLODE1)), LE(SEGA_ANGLE(0)),   0x40,
      #define S_EXPLODE3 26
      0,             LE(1024), LE(1024), LE(V_ADDR(V_EXPLODE1)), LE(SEGA_ANGLE(11)),   0x40,
      #define S_EXPLODE4 27
      0,             LE(1024), LE(1024), LE(V_ADDR(V_EXPLODE1)), LE(SEGA_ANGLE(22)),   0x40,
      #define S_EXPLODE5 28
      0,             LE(1024), LE(1024), LE(V_ADDR(V_EXPLODE1)), LE(SEGA_ANGLE(33)),   0x40,
      #define S_EXPLODE6 29
      0,             LE(1024), LE(1024), LE(V_ADDR(V_EXPLODE1)), LE(SEGA_ANGLE(44)),   0x40,
      #define S_EXPLODE7 30
      0,             LE(1024), LE(1024), LE(V_ADDR(V_EXPLODE1)), LE(SEGA_ANGLE(55)),   0x40,

      #define S_SMOKE0   31
      0,             LE(1024), LE(1024), LE(V_ADDR(V_SMOKE)), LE(SEGA_ANGLE(0)),   0x40,
      #define S_SMOKE1   32
      0,             LE(1024), LE(1024), LE(V_ADDR(V_SMOKE)), LE(SEGA_ANGLE(0)),   0x40,
      #define S_SMOKE2   33
      0,             LE(1024), LE(1024), LE(V_ADDR(V_SMOKE)), LE(SEGA_ANGLE(0)),   0x40,

      #define S_BONUS0    34
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BLANK)),     LE(0),               0x80,
      #define S_BONUS1    35
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BLANK)),     LE(0),               0x80,
      #define S_BONUS2    36
      0,            LE(1024), LE(1024), LE(V_ADDR(V_BLANK)),     LE(0),               0x80,

      #define S_LAST  36
      SEGA_VISIBLE|SEGA_LAST, LE(1024), LE(1024), LE(V_ADDR(V_BLANK)), LE(0), 0x80,
   };
   


// the xy ram is not shadowed and the xy board is running asynchonously 
// from the cpu, so you can't write it without corrupting graphics.
symbol_t *const symbols = (symbol_t*)(VECTOR_RAM); // must be at the top of vector ram

// the vector table is useful to describe vectors, but since scale, position and rotation
// are all set in the symbol drawing list that vector table might as well be in rom
#define VECTOR_RAM_BASE (VECTOR_RAM+SYMBOLS_SZ)
vector_t *const vectors = (vector_t*)(VECTOR_RAM_BASE); // arbitrary location in vector ram

typedef struct {
   int8_t x_speed;
   int8_t y_speed;
   int8_t rotation_speed;
   int8_t resize_speed;
   bool slow; // floating point speeds are tempting
} motion_t;

static motion_t motion[S_LAST+1] = { {0,}, };


static void animate(void) {
   static uint16_t last_tick = 0;
   uint8_t frame = system_tick - last_tick;

   if ( frame == 0 ) return;
   last_tick = system_tick;

   for (uint8_t i=0; i<sizeof(symbol)/sizeof(symbol_t); i++) {
      symbol_t *const sym = &symbols[i];
      if ( sym->visible ) {
         motion_t *m = &motion[ i ];
         if ( m->slow && ((system_tick & 0x0003) != 0) ) continue;
         sym->rotation = (sym->rotation + m->rotation_speed) & 0x03FF;
         uint8_t last_scale = sym->scale;
         if ( m->resize_speed ) {
            sym->scale += m->resize_speed;
            if ( sym->scale < last_scale ) {
               sym->visible = false;
            }
         }
         if ( m->x_speed ) {
            sym->x += m->x_speed;
            if ((m->x_speed > 0 && sym->x > MAX_X) || (m->x_speed < 0 && sym->x < MIN_X)) {
               sym->visible = false;
            }
         }
         if ( m->y_speed ) {
            sym->y += m->y_speed;
            if ((m->y_speed > 0 && sym->y > MAX_Y) || (m->y_speed < 0 && sym->y < MIN_Y)) {
               sym->visible = false;
            }
         }
      }
   }
}

static inline void setTrajectory( uint8_t sid, uint8_t velocity, uint16_t sega_angle ) {
   // compound literals require ISO C99 or later and are not implemented
   // motion[sid] = (motion_t){ .y = 5 };
   motion_t *m = &motion[ sid ];
   int16_t x, y;
   vectorToXY( sega_angle, velocity, &x, &y );
   m->x_speed = x;
   m->y_speed = y;
}

static inline void setRotationSpeed( uint8_t sid, int8_t rotation_speed ) {
   motion_t *m = &motion[ sid ];
   m->rotation_speed = rotation_speed;
}

static inline void setResizeSpeed( uint8_t sid, int8_t resize_speed ) {
   motion_t *m = &motion[ sid ];
   m->resize_speed = resize_speed;
}

static inline void setStop( uint8_t sid ) {
   motion_t *m = &motion[ sid ];
   memset( (uint8_t*)m, 0x00, sizeof(motion_t) );
}

static void vector_init(void) {
   #if S_ADDR(S_LAST) > (VECTOR_RAM_BASE)
       #error 'symbols do not fit in memory'
   #endif
   #if V_ADDR(V_LAST) > (VECTOR_RAM+VECTOR_RAM_SZ)
       #error 'vectors do not fit in memory'
   #endif

   memcpy( (uint8_t*)symbols, symbol, sizeof(symbol) );
   memcpy( (uint8_t*)vectors, vector, sizeof(vector) );

   installFonts( (uint16_t)vectors+sizeof(vector) );
}


static uint16_t spinner_vector_angle( bool reset ) {
   PORT_370 = SELECT_SPINNER;
   delay(1);
   uint8_t value = PORT_374;
   bool dir = value & 0x01;
   value = value >> 1;
   PORT_370 = SELECT_BUTTONS;
   delay(1);

   static uint16_t angle = 0;
   static uint16_t lastvalue = 0;
   if ( reset ) {
      angle = 0;
   } else {
      if ( value > lastvalue ) {
         lastvalue += 127; // 2^7 max angle in spinner space
      }
      uint8_t delta = lastvalue - value;
      // spinner angle in degrees is about 5.6 * value
      // vector is SEGA_ANGLE( angle ), so 2.845 * 5.6 = ~16
      #ifdef MAME_BUILD
         delta >>= 1; // mame seems to increment the spinner inaccurately
      #else
         // seems to work great on real hardware
         delta <<= 4;  // x 16
      #endif
      if (dir) {
         // only ever counts down so we have to account for direction bit
         angle += delta;
      } else {
         angle -= delta;
      }
      angle &= 0x03FF; // 2^10 max angle in vector space
   }
   lastvalue = value;
   return angle;
}

static void timer_interrupt_4Hz(void) {

   switch( game_state ) {
      case game_state_boot:
         break;

      case game_state_attract:
         break;

      case game_state_play: {
         if ( system_tick & BIT(0) ) {
            return;
         }
         // simple break beat
         const uint8_t track0[] = { BASE_DRUM,          0,  SNARE_DRUM,         0,
                                    BASE_DRUM,  BASE_DRUM,  SNARE_DRUM,         0, 
                                    BASE_DRUM,          0,  SNARE_DRUM, BASE_DRUM,
                                            0,  BASE_DRUM,  SNARE_DRUM,         0 };

         // // more complex break beat
         const uint8_t track1[] = { BASE_DRUM,          0,  SNARE_DRUM, BASE_DRUM,
                                    BASE_DRUM,          0,  SNARE_DRUM, BASE_DRUM, 
                                    BASE_DRUM,          0,  SNARE_DRUM, BASE_DRUM,
                                    BASE_DRUM, SNARE_DRUM,  SNARE_DRUM, BASE_DRUM };
         static uint8_t ix = 0;
         if ( sound_track == 0 ) {
            SOUND_COMMAND = track0[ ix ];
         } else {
            SOUND_COMMAND = SNARE_DRUM;
            if (ix==0 || ix==4 || ix==8 || ix==12) {
               if (sound_track) {
                  sound_track--;
               }
            }
         }
         ix++;
         if (ix>=sizeof(track0)) {
            ix = 0;
         }
         break; }

      case game_state_highscore: {
         // new order blue monday
         const uint8_t track[]  = { BASE_DRUM,         0,           0,         0,
                                    BASE_DRUM,         0,           0,         0,
                                    BASE_DRUM,         0,           0,         0,
                                    BASE_DRUM,         0,           0,         0,
                                    BASE_DRUM,         0,           0,         0,
                                    BASE_DRUM,         0,           0,         0,
                                    BASE_DRUM, BASE_DRUM, BASE_DRUM, BASE_DRUM,
                                    BASE_DRUM, BASE_DRUM, BASE_DRUM, BASE_DRUM };
         static uint8_t ix = 0;
         SOUND_COMMAND = track[ ix ];
         ix++;
         if (ix>=sizeof(track)) ix = 0;
         break; }

   }
}


static void colorizeCube( vector_t *const vec, uint8_t quad ) {
   switch ( quad ) {
      case 1: // top right
         vec[1].color = SEGA_COLOR_CYAN;
         vec[2].color = SEGA_COLOR_CYAN2;
         vec[3].color = SEGA_COLOR_CYAN2;
         vec[4].color = SEGA_COLOR_CYAN;
         vec[9].color = SEGA_COLOR_CYAN2;
         vec[11].color = SEGA_COLOR_CYAN;
         vec[12].color = SEGA_COLOR_CYAN;
         vec[17].color = SEGA_COLOR_CYAN|SEGA_LAST;
         break;
      case 2: // top left
         vec[1].color = SEGA_COLOR_CYAN2;
         vec[2].color = SEGA_COLOR_CYAN2;
         vec[3].color = SEGA_COLOR_CYAN;
         vec[4].color = SEGA_COLOR_CYAN;
         vec[9].color = SEGA_COLOR_CYAN;
         vec[11].color = SEGA_COLOR_CYAN2;
         vec[12].color = SEGA_COLOR_CYAN;
         vec[17].color = SEGA_COLOR_CYAN|SEGA_LAST;
         break;
      case 3: // bottom left
         vec[1].color = SEGA_COLOR_CYAN;
         vec[2].color = SEGA_COLOR_CYAN;
         vec[3].color = SEGA_COLOR_CYAN;
         vec[4].color = SEGA_COLOR_CYAN;
         vec[9].color = SEGA_COLOR_CYAN;
         vec[11].color = SEGA_COLOR_CYAN2;
         vec[12].color = SEGA_COLOR_CYAN2;
         vec[17].color = SEGA_COLOR_CYAN2|SEGA_LAST;
         break;
      case 4: // bottom right
         vec[1].color = SEGA_COLOR_CYAN;
         vec[2].color = SEGA_COLOR_CYAN;
         vec[3].color = SEGA_COLOR_CYAN2;
         vec[4].color = SEGA_COLOR_CYAN2;
         vec[9].color = SEGA_COLOR_CYAN;
         vec[11].color = SEGA_COLOR_CYAN;
         vec[12].color = SEGA_COLOR_CYAN;
         vec[17].color = SEGA_COLOR_CYAN2|SEGA_LAST;
         break;
   }
}

static inline uint8_t quadrant( uint16_t x, uint16_t y ) {
   if ( x < 1024 ) {
      if ( y < 1024 ) {
         return 3; // bottom left
      } else {
         return 2; // top left
      }
   } else {
      if ( y < 1024 ) {
         return 4; // bottom right
      } else {
         return 1; // top right
      }
   }
}

static inline void resetSymbol( uint8_t sid, uint16_t x, uint16_t y, uint16_t sega_angle, uint8_t scale ) {
   symbols[sid].x = x;
   symbols[sid].y = y;
   symbols[sid].rotation = sega_angle;
   if ( scale != 0 ) symbols[sid].scale = scale;
}

static void enableSymbol( uint8_t sid, uint16_t x, uint16_t y, uint16_t sega_angle, uint8_t scale ) {
   resetSymbol( sid, x, y, sega_angle, scale );
   symbols[sid].visible = true;
}

static void moveObjectsX( void ) {
   symbols[S_BONUS].x -= 1;

   // weird, we didn't have these two, makes game for strategic, but odder having them - 
   // cause you can evade chopper with movement.
   symbols[S_CHOPPER].x -= 1;
   symbols[S_BLADE].x -= 1;

   for (uint8_t i=0; i<3; i++) {
      symbols[S_CUBE0+i].x -= 1;
      if ( symbols[S_CUBE0+i].x < 250 ) {
         symbols[S_CUBE0+i].x = 1024+500;
      }
   }
}

static void moveObjectsY( void ) {
   symbols[S_BONUS].y -= 1;
   symbols[S_CHOPPER].y -= 1;
   symbols[S_BLADE].y -= 1;
   for (uint8_t i=0; i<3; i++) {
      symbols[S_CUBE0+i].y -= 1;
      if ( symbols[S_CUBE0+i].y < 250 ) {
         symbols[S_CUBE0+i].y = 1024+500;
      }
   }
}


static void skewCubes( bool reset ) {
   static uint8_t last_quad[3] = {0xFF,0xFF,0xFF};
   static uint8_t ct = 0;
   if ((++ct<9) && (!reset)) return;
   ct = 0;

   static uint8_t ix = 0;
   ix++;
   if (ix>2) ix = 0;

   const uint16_t vid[] = { V_CUBE0, V_CUBE1, V_CUBE2 };
   vector_t *const vec = &vectors[ vid[ix] ];
   symbol_t *const sym = &symbols[ S_CUBE0 + ix ];

   uint8_t quad = quadrant( sym->x, sym->y );
   if ( quad != last_quad[ ix ] || reset ) {
      colorizeCube( vec, quad );
      last_quad[ ix ] = quad;
   }

   uint16_t _a = xyToVector( sym->x, sym->y );
   uint16_t a = _a + SEGA_ANGLE(180) & 0x3FF;
   vec[1].angle = a;
   vec[4].angle = _a;
   vec[9].angle = a;
   vec[12].angle = _a;
   vec[15].angle = a;
}

static void startChopper(void) {
   uint8_t q = rand()&3;
   switch ( q ) {
      case 0:
         enableSymbol( S_CHOPPER, MIN_X, 1024, SEGA_ANGLE(180), 0 );
         enableSymbol( S_BLADE, MIN_X, 1024, SEGA_ANGLE(0), 0 );
         setTrajectory( S_CHOPPER, 5, SEGA_ANGLE(90) );
         setTrajectory( S_BLADE, 5, SEGA_ANGLE(90) );
         break;
      case 1:
         enableSymbol( S_CHOPPER, MAX_X, 1024, SEGA_ANGLE(0), 0 );
         enableSymbol( S_BLADE, MAX_X, 1024, SEGA_ANGLE(0), 0 );
         setTrajectory( S_CHOPPER, 5, SEGA_ANGLE(270) );
         setTrajectory( S_BLADE, 5, SEGA_ANGLE(270) );
         break;
      case 2:
         enableSymbol( S_CHOPPER, 1024, MIN_Y, SEGA_ANGLE(90), 0 );
         enableSymbol( S_BLADE, 1024, MIN_Y, SEGA_ANGLE(0), 0 );
         setTrajectory( S_CHOPPER, 5, SEGA_ANGLE(0) );
         setTrajectory( S_BLADE, 5, SEGA_ANGLE(0) );
         break;
      case 3:
         enableSymbol( S_CHOPPER, 1024, MAX_Y, SEGA_ANGLE(270), 0 );
         enableSymbol( S_BLADE, 1024, MAX_Y, SEGA_ANGLE(0), 0 );
         setTrajectory( S_CHOPPER, 5, SEGA_ANGLE(180) );
         setTrajectory( S_BLADE, 5, SEGA_ANGLE(180) );
         break;
   }
   setRotationSpeed( S_BLADE, SEGA_ANGLE(18) );
}


static void drawTank(uint16_t angle) {
   static uint16_t last_angle = 0;
   if ( angle != last_angle ) {
      last_angle = angle;
      symbols[ S_BARREL ].rotation = angle;
      symbols[ S_TURRET ].rotation = angle;
      symbols[ S_FLAME ].rotation = angle;
   }
}


static void resetVectors( void ) {
   memset( (uint8_t*)motion, 0x00, sizeof(motion) );
   memcpy( (uint8_t*)vectors, vector, sizeof(vector) ); // reset colorized vectors
   memcpy( (uint8_t*)symbols, symbol, sizeof(symbol) ); // reset symbols
}

static void beginAttract( void ) {
   resetVectors();

   // disable all other symbols on screen
   // interestingly, hardware ignores last flag if it's not also visible
   symbols[S_STRING].flags = SEGA_VISIBLE|SEGA_LAST;
}


static void endAttract( void ) {
   resetVectors();

   symbols[ S_TANK ].visible = true;
   symbols[ S_TREAD ].visible = true;
   symbols[ S_TURRET ].visible = true;
   symbols[ S_BARREL ].visible = true;
   symbols[ S_CUBE0 ].visible = true;
   symbols[ S_CUBE1 ].visible = true;
   symbols[ S_CUBE2 ].visible = true;
   symbols[ S_BONUS ].visible = true;
   symbols[ S_STREET ].visible = true;

   // set font 'a' thru 'z' to regular white
   colorize( (uint8_t*)fontAddress('a'), fontAddress('z'+1)-fontAddress('a'), SEGA_COLOR_WHITE );

   // set numbers '0' thru '9' to pink
   colorize( (uint8_t*)fontAddress('0'), fontAddress('9'+1)-fontAddress('0'), SEGA_COLOR_MAGENTA );

   // clear last flag, enable all other symbols on screen
   symbols[S_STRING].flags &= ~SEGA_LAST;
}


static bool drawAttract( void ) {

   if ( _coin_counter ) {
      if ( (PORT_374 & BUTTON_PLAYER_1) ) {
         _coin_counter--;
         return true;
      }
   }

   static uint8_t state = 0;
   static uint8_t state_ix = 0;
   static uint16_t state_iy = 0;
   static uint16_t last_tick = 0;

   static uint8_t last_coin_counter = 0;
   uint16_t coin_counter = _coin_counter;
   if ( coin_counter != last_coin_counter ) {
      last_coin_counter = coin_counter;
      state = 2;
   }

   switch ( state ) {

      case 0: {
         const char s[] = "game over";
         drawString( &symbols[S_STRING], CENTER_X-155, MIN_Y+40, 0x80, SEGA_COLOR_YELLOW, s, sizeof(s)-1 );
         last_tick = system_tick;
         // set font 'a' thru 'z' to brightest white for phosphor afterglow
         colorize( (uint8_t*)fontAddress('a'), fontAddress('z'+1)-fontAddress('a'), SEGA_COLOR_BRWHITE );
         state++;
         break; }

      case 2:
         setStop( S_STRING );
         if ( _coin_counter > 0 ) {
            const char s[] = "press start";
            drawString( &symbols[S_STRING], CENTER_X-165, MIN_Y+40, 0x80, SEGA_COLOR_BLUE, s, sizeof(s)-1 );
         } else {
            const char s[] = "insert coin";
            drawString( &symbols[S_STRING], CENTER_X-165, MIN_Y+40, 0x80, SEGA_COLOR_GREEN, s, sizeof(s)-1 );
         }
         last_tick = system_tick;
         // set font 'a' thru 'z' to brightest white for phosphor afterglow
         colorize( (uint8_t*)fontAddress('a'), fontAddress('z'+1)-fontAddress('a'), SEGA_COLOR_BRWHITE );
         state++;
         break;

      case 1: 
      case 3: {
         const char str[] = "attack vektor";
         symbol_t *const sym = &symbols[S_SCORE0];
         sym->x = MIN_X-70;
         sym->y = CENTER_Y;
         sym->scale = 0xFF;
         for ( uint8_t i=0; i<sizeof(str); i++ ) {
            // draw as fast as possible 
            // but in step with vector XY redraw which we're sensing indirectly though system_tick IRQ
            static uint16_t last_tick = 0;
            while ( system_tick == last_tick ) {
               __asm__( "nop" );
            }
            last_tick = system_tick;

            sym->x += 70;
            if ( str[i] == ' ' ) {
               sym->visible = false;
            } else {
               sym->vector_addr = fontAddress( str[i] );
               sym->visible = true;
            }
         }
         if ( system_tick - last_tick > SECONDS(3) ) {
            sym->visible = false;
            state_ix = 2;
            state_iy = CENTER_Y-250;
            state++;
         }
         break; }

      case 4:
      case 7:
      case 10: {
         char s[7] = {0,};
         memcpy( &s[0], high_name[state_ix], 3 );
         digits3( &s[4], &s[5], &s[6], high_score[state_ix] );
         colorize( (uint8_t*)fontAddress('a'), fontAddress('z'+1)-fontAddress('a'), SEGA_COLOR_CYAN );
         colorize( (uint8_t*)fontAddress('0'), fontAddress('9'+1)-fontAddress('0'), SEGA_COLOR_WHITE );
         drawString( &symbols[S_STRING], 0, state_iy, 0xA0, 0, s, sizeof(s) );
         setTrajectory( S_STRING, 20, SEGA_ANGLE(90) );
         state_ix--;
         state_iy += 250;
         state++;
         break; }

      case 5:
      case 8:
      case 11:
         if ( symbols[S_STRING].x > CENTER_X-150 ) {
            setStop( S_STRING );
            last_tick = system_tick;
            state++;
         }
         break;

      case 6:
      case 9:
      case 12:
         if ( system_tick - last_tick > SECONDS(1) ) {
            state++;
         }
         break;

      default:
         state = 0;
         break;
    }

   return false;
}


static uint8_t high_index = 0;

static void beginDrawInitials( void ) {
   say( HIGH_SCORE );
   enableSymbol( S_NAME0, CENTER_X-70, CENTER_Y-220, SEGA_ANGLE(0), 0xA0 );
   enableSymbol( S_NAME1, CENTER_X-10, CENTER_Y-220, SEGA_ANGLE(0), 0xA0 );
   enableSymbol( S_NAME2, CENTER_X+50, CENTER_Y-220, SEGA_ANGLE(0), 0xA0 );
   spinner_vector_angle( true );

   if ( score >= high_score[0] ) {
      high_index = 0;
      high_score[ 2 ] = high_score[ 1 ];
      high_score[ 1 ] = high_score[ 0 ];
      high_score[ 0 ] = score;
      memcpy( high_name[2], high_name[1], 3 );
      memcpy( high_name[1], high_name[0], 3 );
      memset( high_name[0], 0x00, 3 );
      return;
   }
   if ( score >= high_score[1] ) {
      high_index = 1;
      high_score[ 2 ] = high_score[ 1 ];
      high_score[ 1 ] = score;
      memcpy( high_name[2], high_name[1], 3 );
      memset( high_name[1], 0x00, 3 );
      return;
   }
   if ( score >= high_score[2] ) {
      high_index = 2;
      high_score[ 2 ] = score;
      memset( high_name[2], 0x00, 3 );
      return;
   }
}


static bool drawInitials( void ) {
   static uint8_t ix = 0;
   static uint16_t *addr[] = {  &symbols[S_NAME0].vector_addr,
                                &symbols[S_NAME1].vector_addr,
                                &symbols[S_NAME2].vector_addr };

   uint16_t vec_angle = spinner_vector_angle( false );
   char ch = 'a' + div_16( vec_angle, 39 ); // 2^10 / 26
   ch = MIN( MAX(ch, 'a'), 'z');
   static char last_ch = 0;
   if ( ch != last_ch ) {
      last_ch = ch;
      *addr[ix] = fontAddress( ch );
   } else {
      // blink cursor
      static uint16_t last_tick = 0;
      if ( (system_tick - last_tick) > 10 ) {
         last_tick = system_tick;
         if ( *addr[ix] == V_ADDR(V_LINE) ) {
            *addr[ix] = fontAddress( ch );
         } else {
            *addr[ix] = V_ADDR(V_LINE);
         }
      }
   }

   // debounce and advance next letter
   static uint16_t last_button_tick = 0; 
   uint8_t buttons = PORT_374;
   if ((buttons & BUTTON_FIRE) && ((system_tick - last_button_tick) > 50)) {
      last_button_tick = system_tick;
      high_name[  high_index ][ ix ] = ch;
      *addr[ix] = fontAddress( ch );
      ix++;
      if (ix == 3) {
         ix = 0;
         say( CONGRATULATIONS );
         uint16_t last_tick = system_tick;
         // rainbow effect
         for (uint8_t color=0; color<0x3F; color++) {
            // synchronize color to the vector XY redraw
            static uint16_t last_tick = 0;
            while ( system_tick == last_tick ) {
               __asm__( "nop" );
            }
            last_tick = system_tick;
            colorize( (uint8_t*)fontAddress('a'), fontAddress('z'+1)-fontAddress('a'), (color&0x3F) << 1 );
         }
         return true;
      }
   }
   return false;
}

void drawScore( uint8_t score, bool reset ) {
   // unpacked bcd style
   // fast and no division but can lose track of real value
   static uint8_t last_score = 0;
   static char d0, d1, d2 = 0;
   if ( reset ) {
      d0 = d1 = d2 = '0';
      last_score = ~score;
   } 
   if ( score != last_score ) {
      last_score = score;
      if ( !reset ) {
         // when parameter changed assume it counted UP by one
         if (++d2 > '9') {
            d2 = '0';
            if (++d1 > '9') {
               d1 = '0';
               if (++d0 > '9') {
                  d0 = '0';
               }
            }
         }
      }
      symbols[ S_SCORE0 ].vector_addr = fontAddress( d0 );
      symbols[ S_SCORE1 ].vector_addr = fontAddress( d1 );
      symbols[ S_SCORE2 ].vector_addr = fontAddress( d2 );
   }
   if ( reset ) {
      symbols[ S_SCORE0 ].visible = true;
      symbols[ S_SCORE1 ].visible = true;
      symbols[ S_SCORE2 ].visible = true;
   }
}

void drawBonus(uint8_t value, bool reset) {
  static uint8_t last_value = 0xFF;

  if (reset) last_value = 0xFF;

  if (value != last_value) {
    last_value = value;

    if (value == 0) {
      symbols[S_BONUS0].visible = false;
      symbols[S_BONUS1].visible = false;
      symbols[S_BONUS2].visible = false;
      return;
    }

    symbols[S_BONUS0].visible = true;
    symbols[S_BONUS1].visible = true;
    symbols[S_BONUS2].visible = true;

    uint8_t tens = 0;
    uint8_t ones = value;
    while (ones >= 10) { ones -= 10; tens++; }

    uint8_t d0 = ' ';
    uint8_t d1 = '0' + tens;
    uint8_t d2 = '0' + ones;

    symbols[S_BONUS0].vector_addr = fontAddress(d0);
    symbols[S_BONUS1].vector_addr = fontAddress(d1);
    symbols[S_BONUS2].vector_addr = fontAddress(d2);
  }
}


static void shrapnel( symbol_t *const sym ) {
   for (uint8_t sid=S_EXPLODE2, q=0; sid<=S_EXPLODE7; sid++,q++) {
      uint8_t scale = 0x30 + (q<<4);
      uint8_t velocity = 1 + 30-(q<<1);
      setTrajectory( sid, velocity, randSegaAngle() );
      setRotationSpeed( sid, SEGA_ANGLE(18) );
      enableSymbol( sid, sym->x, sym->y, SEGA_ANGLE(0), scale );
   }
}

static void explode( symbol_t *const sym ) {
   setRotationSpeed( S_EXPLODE0, SEGA_ANGLE(12) );
   setResizeSpeed( S_EXPLODE0, 25 );
   enableSymbol( S_EXPLODE0, sym->x, sym->y, SEGA_ANGLE(0), 0x30 );
   setRotationSpeed( S_EXPLODE1, SEGA_ANGLE(-12) );
   setResizeSpeed( S_EXPLODE1, 35 );
   enableSymbol( S_EXPLODE1, sym->x, sym->y, SEGA_ANGLE(0), 0x10 );
}

static inline bool checkColission( symbol_t *const s0, symbol_t *const s1 ) {
   uint16_t x0 = s0->x-(HITBOX_SZ/2);
   uint16_t x1 = s0->x+(HITBOX_SZ/2);
   uint16_t y0 = s0->y-(HITBOX_SZ/2);
   uint16_t y1 = s0->y+(HITBOX_SZ/2);
   return ( s1->x > x0 && s1->x < x1 && s1->y > y0 && s1->y < y1 );
}


static uint8_t street_state;
static int16_t street_y;
static int16_t street_x;
static uint16_t chopper_timer;
static uint8_t bonus_count;

static void beginPlay(void) {
   street_state = 0;
   street_y = (200+(255*2));
   street_x = 0;
   chopper_timer = system_tick;
   bonus_count = 0;
   score = 0;
   missle_max = 1;
   vectors[ V_BARREL + 8 ].last = true;
   resetSymbol( S_SCORE0, CENTER_X-40, MIN_Y+10, SEGA_ANGLE(0), 0x80 );
   resetSymbol( S_SCORE1, CENTER_X,    MIN_Y+10, SEGA_ANGLE(0), 0x80 );
   resetSymbol( S_SCORE2, CENTER_X+40, MIN_Y+10, SEGA_ANGLE(0), 0x80 );
   resetSymbol( S_BONUS0, CENTER_X-20, MAX_Y-10, SEGA_ANGLE(0), 0x40 );
   resetSymbol( S_BONUS1, CENTER_X,    MAX_Y-10, SEGA_ANGLE(0), 0x40 );
   resetSymbol( S_BONUS2, CENTER_X+20, MAX_Y-10, SEGA_ANGLE(0), 0x40 );
   drawScore(score, true);
   spinner_vector_angle( true );
   skewCubes(true);
   skewCubes(true);
   skewCubes(true);
}


static bool drawPlay(void) {
   symbol_t *const chopper = &symbols[S_CHOPPER];
   symbol_t *const tank = &symbols[S_TANK];
   symbol_t *const bonus = &symbols[S_BONUS];

   uint16_t vec_angle = spinner_vector_angle( false );
   static uint8_t missle_dist = 0;

   uint8_t buttons = PORT_374;

   static uint8_t ct=0;
   if ( ct == 0 ) {
      if ( buttons & BUTTON_FIRE ) {
         for (uint8_t i=0; i<missle_max; i++) {
            uint8_t sym_ix = S_MISSLE0+i;
            symbol_t *const missle = &symbols[sym_ix];
            if ( !missle->visible ) {
               int16_t x=0, y=0;
               vectorToXY( vec_angle, 80, &x, &y ); // offset to barrel's tip
               enableSymbol( sym_ix, CENTER_X+x, CENTER_Y+y, vec_angle, 0 );
               setTrajectory( sym_ix, 8, vec_angle );
               ct = 30;
               SOUND_COMMAND = TANK_FIRE;
               symbols[ S_FLAME ].visible = true;
               if (bonus_count) {
                  bonus_count--;
                  drawBonus( bonus_count, false );
                  if ( bonus_count == 0 ) {
                     bonus->visible = true;
                     missle_max = 1;
                     vectors[ V_BARREL + 8 ].last = true; // disable triple stripe barrel
                  }
               }
               break;
            }
         }
      }
   }
   if (ct>=30) {
      ct++; 
      vectors[ V_BARREL ].size = ct;
   }
   if (ct==45) {
      symbols[ S_FLAME ].visible = false;
   }
   if (ct==60) {
      vectors[ V_BARREL ].size = SIZE(5);
      ct = 0;
   }

   drawTank( vec_angle );


   if ( chopper->visible ) {

#ifndef GOD_MODE
      if ( checkColission( chopper, tank ) ) {
         symbols[ S_FLAME ].visible = false;
         symbols[ S_MISSLE0 ].visible = false;
         symbols[ S_MISSLE1 ].visible = false;
         symbols[ S_MISSLE2 ].visible = false;
         explode( tank );
         SOUND_COMMAND = TANK_EXPLODE;
         return true; // game over
      }
#endif

      for (uint8_t i=0; i<3; i++) {
         uint8_t sym_ix = S_MISSLE0+i;
         symbol_t *const missle = &symbols[sym_ix];
         if ( missle->visible ) {
            if ( checkColission( chopper, missle ) ) {
               score++;
               missle->visible = false;
               symbols[ S_CHOPPER ].visible = false;
               symbols[ S_BLADE ].visible = false;
               explode( chopper );
               shrapnel( chopper );
               break;
            }
         }
      }

   } else {

      if ( system_tick - chopper_timer >= 50 - MIN(50,score) ) {
         chopper_timer = system_tick;
         startChopper();
      }

   }

   if ( bonus->visible ) {
      if ( checkColission( bonus, tank ) ) {
         bonus->visible = false;
         SOUND_COMMAND = BONUS_START;
         missle_max = 3;
         vectors[ V_BARREL + 8 ].last = false; // triple stripe barrel
         bonus_count = 30;
         drawBonus(bonus_count, true);
      }
   }


   // symbols aren't drawn if their origin point is off screen
   // but the hardware will clip big symbol vectors outside the view.
   // so move the street by changing the invisible offset vector at the
   // begining of the symbol and just leave origin at CENTER screen

   vector_t *const vec = &vectors[ V_STREET ];
   motion_t *const m = &motion[ S_TANK ];
   uint16_t angle = symbols[ S_TANK ].rotation;

   if ( buttons & BUTTON_THRUST ) {
      SOUND_COMMAND = TANK_MOVE;
      static uint8_t l = 10;
      vectors[ V_TREAD+1 ].size = l;
      l += 1;
      if ( l >40 ) l = 10;
   }

   switch ( street_state ) {
      case 0:
         // moving up screen
         if ( buttons & BUTTON_THRUST ) {
            if ( street_y > 0 ) {
               stretch3( &vec[0].size, street_y );
               street_y -= 1;
               moveObjectsY();
               skewCubes( false );
            } else {
               setRotationSpeed( S_TANK, SEGA_ANGLE(12) );
               setRotationSpeed( S_TREAD, SEGA_ANGLE(12) );
               street_state++;
            }
         }
         break;
      case 1:
         // turning to right
         if ( angle >= SEGA_ANGLE(90) ) {
            setRotationSpeed( S_TANK, 0 );
            setRotationSpeed( S_TREAD, 0 );
            symbols[ S_TANK ].rotation = SEGA_ANGLE(90);
            symbols[ S_TREAD ].rotation = SEGA_ANGLE(90);
            street_state++;
         }
         break;
      case 2:
         // moving right
         if ( buttons & BUTTON_THRUST ) {
            if ( street_x < (255*5) ) {
               stretch5( &vec[4].size, street_x );
               street_x += 1;
               moveObjectsX();
               skewCubes( false );
            } else {
               setRotationSpeed( S_TANK, SEGA_ANGLE(-12) );
               setRotationSpeed( S_TREAD, SEGA_ANGLE(-12) );
               street_state++;
            }
         }
         break;
      case 3:
         // turning to left
         if ( angle > SEGA_ANGLE(360-45) ) {
            setRotationSpeed( S_TANK, 0 );
            setRotationSpeed( S_TREAD, 0 );
            symbols[ S_TANK ].rotation = SEGA_ANGLE(0);
            symbols[ S_TREAD ].rotation = SEGA_ANGLE(0);
            vec[0].angle = SEGA_ANGLE(180);
            vec[1].angle = SEGA_ANGLE(180);
            vec[2].angle = SEGA_ANGLE(180);
            street_state++;
         }
         break;
      case 4:
         // moving up
         if ( buttons & BUTTON_THRUST ) {
            if ( street_y < (570) ) {
               stretch3( &vec[0].size, street_y );
               street_y += 1;
               moveObjectsY();
               skewCubes( false );
            } else {
               // reset
               resetSymbol( S_CUBE0, CUBE_X0, CUBE_Y0, 0, 0xe0 );
               resetSymbol( S_CUBE1, CUBE_X1, CUBE_Y1, 0, 0xe0 );
               resetSymbol( S_CUBE2, CUBE_X0, CUBE_Y2, 0, 0xe0 );
               vec[0].angle = SEGA_ANGLE(0);
               vec[1].angle = SEGA_ANGLE(0);
               vec[2].angle = SEGA_ANGLE(0);
               street_x = 0;
               street_y = 200+(255*2);
               stretch3( &vec[0].size, street_y );
               stretch5( &vec[4].size, street_x );
               street_state = 0;
            }
         }
         break;
   }

   return false;
}

static void keepPuffing(void) {
   static uint16_t last_tick = 0;
   uint16_t frame = system_tick - last_tick;
   if ( frame >= SECONDS(1.5) ) {
      last_tick = system_tick;
      static uint8_t sid = S_SMOKE0;
      if ( ++sid > S_SMOKE2 ) sid = S_SMOKE0;
      enableSymbol( sid, CENTER_X, CENTER_Y, SEGA_ANGLE(0), 16 );
   }
}

static void beginGameOver(void) {
   if ( score <= high_score[2] ) {
      const char s[] = "game over";
      drawString( &symbols[S_STRING], CENTER_X-280, MIN_Y, 0xFE, SEGA_COLOR_RED, s, sizeof(s)-1 );
   }  else {
      const char s[] = "high score";
      drawString( &symbols[S_STRING], CENTER_X-280, MIN_Y, 0xFE, SEGA_COLOR_YELLOW, s, sizeof(s)-1 );
   }

   colorize( (uint8_t*)&vectors[V_SMOKE], V_SMOKE_LEN*sizeof(vector_t), SEGA_COLOR_WHITE );

   setTrajectory( S_STRING, 5, SEGA_ANGLE(0) );

   uint16_t a = randSegaAngle();
   setTrajectory( S_TURRET, 6, a );
   setRotationSpeed( S_TURRET, SEGA_ANGLE(7) );
   setTrajectory( S_BARREL, 7, a+SEGA_ANGLE(90) );
   setRotationSpeed( S_BARREL, -SEGA_ANGLE(5) );

   for (uint8_t i=0; i<3; i++) {
      uint8_t sid = S_SMOKE0 + i;
      setRotationSpeed( sid, 1 );
      setTrajectory( sid, 2, SEGA_ANGLE(22) );
      setResizeSpeed( sid, 1 );
      motion[ sid ].slow = true;
   }
}

static bool drawGameOver(void) {
   // wait for text to slide into place
   if ( symbols[S_STRING].y > MAX_Y - 120 ) {
      setStop( S_STRING );
      return true;
   }
   return false;
}

static void beginDiagnosticsIO( void ) {
   resetVectors();
   symbols[S_STRING].flags = SEGA_VISIBLE|SEGA_LAST;
   colorize( (uint8_t*)fontAddress('a'), fontAddress('z'+1)-fontAddress('a'), SEGA_COLOR_WHITE );
   colorize( (uint8_t*)fontAddress('0'), fontAddress('9'+1)-fontAddress('0'), SEGA_COLOR_WHITE );
   enableSymbol( S_SCORE0, CENTER_X-70, CENTER_Y-40, SEGA_ANGLE(0), 0xA0 );
   enableSymbol( S_SCORE1, CENTER_X-10, CENTER_Y-40, SEGA_ANGLE(0), 0xA0 );
   enableSymbol( S_SCORE2, CENTER_X+50, CENTER_Y-40, SEGA_ANGLE(0), 0xA0 );
}

static void drawDiagnosticsIO( void ) {
   uint16_t angle = spinner_vector_angle(false);
   const char *hex = "0123456789abcdef";
   uint8_t d0 = hex[ (angle >> 8) & 0x0F ];
   uint8_t d1 = hex[ (angle >> 4) & 0x0F ];
   uint8_t d2 = hex[ (angle >> 0) & 0x0F ];
   symbols[ S_SCORE0 ].vector_addr = fontAddress( d0 );
   symbols[ S_SCORE1 ].vector_addr = fontAddress( d1 );
   symbols[ S_SCORE2 ].vector_addr = fontAddress( d2 );

   static uint8_t last_f8 = 0xFF;
   uint8_t f8 = PORT_370;
   if ( f8 != last_f8 ) {
      last_f8 = f8;
      static uint8_t s[8];
      s[0] = (f8 & BIT(0)) ? '1' : '0';
      s[1] = (f8 & BIT(1)) ? '1' : '0';
      s[2] = (f8 & BIT(2)) ? '1' : '0';
      s[3] = (f8 & BIT(3)) ? '1' : '0';
      s[4] = (f8 & BIT(4)) ? '1' : '0';
      s[5] = (f8 & BIT(5)) ? '1' : '0';
      s[6] = (f8 & BIT(6)) ? '1' : '0';
      s[7] = (f8 & BIT(7)) ? '1' : '0';
      drawString( &symbols[S_STRING], CENTER_X-100, CENTER_Y+40, 0x80, SEGA_COLOR_WHITE, s, 8 );
   }
}

static void writeVec( vector_t *const vec, uint8_t size, uint8_t color, bool last ) {
   memset( (uint8_t*)vec, 0x00, sizeof(vector_t) );
   vec->color = color;
   vec->visible = true;
   vec->last = last;
   vec->size = size;
   vec->angle = 0;
}

static void beginDiagnosticsGrid( void ) {
   // MAX_X-MIN_X = 900
   writeVec( &vectors[0], 240, SEGA_COLOR_RED, 0 );
   writeVec( &vectors[1], 240, SEGA_COLOR_BLUE, 0 );
   writeVec( &vectors[2], 240, SEGA_COLOR_GREEN, 0 );
   writeVec( &vectors[3], 180, SEGA_COLOR_WHITE, 1 );

   // MAX_Y-MIN_Y = 800
   writeVec( &vectors[4], 255, SEGA_COLOR_YELLOW, 0 );
   writeVec( &vectors[5],   9,  SEGA_COLOR_YELLOW, 0 );
   writeVec( &vectors[6], 255, SEGA_COLOR_CYAN, 0 );
   writeVec( &vectors[7],   9,  SEGA_COLOR_CYAN, 0 );
   writeVec( &vectors[8], 255, SEGA_COLOR_MAGENTA, 0 );
   writeVec( &vectors[9],  10,  SEGA_COLOR_MAGENTA, 1 );

   symbol_t * sym = symbols;
   for (uint16_t y=MIN_Y; y<MAX_Y+1; y+=(MAX_Y-MIN_Y)/12) {
      sym->visible = true;
      sym->last = false;
      sym->x = MIN_X;
      sym->y = y;
      sym->vector_addr = (uint16_t)&vectors[0];
      sym->rotation = SEGA_ANGLE(90);
      sym->scale = 0x80;
      sym++;
   }

   for (uint16_t x=MIN_X; x<MAX_X+1; x+=(MAX_X-MIN_X)/15) {
      sym->visible = true;
      sym->last = false;
      sym->x = x;
      sym->y = MIN_Y;
      sym->vector_addr = (uint16_t)&vectors[4];
      sym->rotation = SEGA_ANGLE(0);
      sym->scale = 0x80;
      sym++;
   }

   (sym-1)->last = true;
}

static void drawDiagnosticsGrid( void ) {
}



static void super_loop(void) {
      static uint16_t last_tick = 0;

      animate();

      switch( game_state ) {

         case game_state_boot:
            beginAttract();
            game_state++;
            break;

         case game_state_attract:
            if ( drawAttract() ) {
               endAttract();
               beginPlay();
               game_state++;
            }
            break;

         case game_state_play:
            drawScore( score, false );
            if ( drawPlay() ) {
               game_state = game_state_game_over;
               beginGameOver();
            }
            break;

         case game_state_game_over:
            if ( drawGameOver() ) {
               if ( score < high_score[2] ) {
                  last_tick = system_tick;
                  game_state = game_state_game_over_pause;
               } else {
                  game_state = game_state_highscore;
                  beginDrawInitials();
               }
            } else {
               keepPuffing();
            }
            break;

         case game_state_game_over_pause:
            if ( system_tick - last_tick > SECONDS(4) ) {
               game_state = game_state_boot;
            } else {
               keepPuffing();
            }
            break;

         case game_state_highscore:
            if ( drawInitials() ) {
               game_state = game_state_boot;
            } else {
               keepPuffing();
            }
            break;

         case game_state_diagnostics_io_init:
            beginDiagnosticsIO();
            game_state = game_state_diagnostics_io;
            break;

         case game_state_diagnostics_io:
            drawDiagnosticsIO();
            break;

         case game_state_diagnostics_grid_init:
            beginDiagnosticsGrid();
            game_state = game_state_diagnostics_grid;
            break;

         case game_state_diagnostics_grid:
            drawDiagnosticsGrid();
            break;


      }
}



static void init(void) {

   SPEECH_CONTROL = 0x28;
   SPEECH_COMMAND = 0x00;
   SPEECH_COMMAND = 0x80;
   SOUND_COMMAND = 0xFF; // 8035 in reset and assert RAM LOAD latch

   // // blank the screen and clear vector ram
   const uint8_t s[] = { SEGA_LAST, LE(1024), LE(1024), LE(VECTOR_RAM+10), LE(0), 0x80,
                         SEGA_CLEAR|SEGA_LAST, 0x80, LE(0) };
   memcpy( (uint8_t*)VECTOR_RAM, s, sizeof(s) );
   memset( (uint8_t*)VECTOR_RAM+14, 0x00, VECTOR_RAM_SZ-14 );

   XY_INIT = 0x04;

   PORT_371 = 0x00;
   PORT_370 = SELECT_BUTTONS;
}



void main(void) {

   init();

   sound_init();

   vector_init();

   __asm__("ei");
   __asm__("halt");

   for (;;) {
      super_loop();
   }

}

