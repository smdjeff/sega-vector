
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


const uint8_t vector[] = {
   #define V_BLANK (0)
   SEGA_COLOR_GRAY|SEGA_LAST,    0, LE(SEGA_ANGLE(0)),

   #define V_LINE  (V_BLANK+1)
   SEGA_CLEAR,                   0x25, LE(SEGA_ANGLE(130)),
   SEGA_COLOR_YELLOW|SEGA_LAST,  0x1C, LE(SEGA_ANGLE(270)),

   #define HITBOX_SZ 80
   #define V_BOX (V_LINE+2)
   SEGA_CLEAR,                       (HITBOX_SZ/1.4),  LE(SEGA_ANGLE(225)),
   SEGA_COLOR_MAGENTA,               HITBOX_SZ,        LE(SEGA_ANGLE(0)),
   SEGA_COLOR_MAGENTA,               HITBOX_SZ,        LE(SEGA_ANGLE(90)),
   SEGA_COLOR_MAGENTA,               HITBOX_SZ,        LE(SEGA_ANGLE(180)),
   SEGA_COLOR_MAGENTA|SEGA_LAST,     HITBOX_SZ,        LE(SEGA_ANGLE(270)),

    // #define V_LOGO (V_BOX+5)
    // #include "art/logo.h"
    #define V_HEAD (V_BOX+5)
    // #define V_HEAD (V_LOGO+V_LOGO_SZ)
    #include "art/head.h"

    #define V_SHIRT (V_HEAD+V_HEAD_SZ)
    #include "art/shirt.h"

    #define V_EXPRESSION1 (V_SHIRT+V_SHIRT_SZ)
    #include "art/expression1.h"

    #define V_HAND (V_EXPRESSION1+V_EXPRESSION1_SZ)
    #include "art/hand.h"

    #define V_LAST (V_HAND+V_HAND_SZ)
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

      #define S_LOGO    7
      0, LE(1024), LE(1024), LE(V_ADDR(V_BOX)), LE(SEGA_ANGLE(0)),   0x80,

      #define S_HEAD    8
      0, LE(1024), LE(1024), LE(V_ADDR(V_HEAD)), LE(SEGA_ANGLE(0)),   0x40,

      #define S_SHIRT   9
      0, LE(1024), LE(1024), LE(V_ADDR(V_SHIRT)), LE(SEGA_ANGLE(0)),   0x40,

      #define S_EXPRESSION1   10
      0, LE(1024), LE(1024), LE(V_ADDR(V_EXPRESSION1)), LE(SEGA_ANGLE(0)),   0x40,

      #define S_HAND      11
      0, LE(1024), LE(1024), LE(V_ADDR(V_HAND)), LE(SEGA_ANGLE(0)),   0x40,

      #define S_LAST      12
      SEGA_VISIBLE|SEGA_LAST, LE(1024), LE(1024), LE(V_ADDR(V_BLANK)), LE(0), 0x80,
   };

// the xy ram is not shadowed and the xy board is running asynchonously 
// from the cpu, so you can't write it without corrupting graphics.
symbol_t *const symbols = (symbol_t*)(VECTOR_RAM); // must be at the top of vector ram

// the vector table is useful to describe vectors, but since scale, position and rotation
// are all set in the symbol drawing list that vector table might as well be in rom
#define VECTOR_RAM_BASE (VECTOR_RAM+SYMBOLS_SZ)
vector_t *const vectors = (vector_t*)(VECTOR_RAM_BASE); // arbitrary location in vector ram




   

void drawHand(uint16_t vec_angle) {
   int16_t dx, dy;
   vectorToXY(vec_angle, 22, &dx, &dy);
   symbols[S_HAND].x = CENTER_X + 200 + dx;
   symbols[S_HAND].y = CENTER_Y - 100 - dy;
   symbols[S_HAND].rotation = (SEGA_ANGLE(90) + (dy >> 2)) & 0x03FF;
}





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
         sym->y += m->y_speed;
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

      case game_state_play:
         break;

      case game_state_highscore:
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



static void resetVectors( void ) {
   memset( (uint8_t*)motion, 0x00, sizeof(motion) );
   memcpy( (uint8_t*)vectors, vector, sizeof(vector) ); // reset colorized vectors
   memcpy( (uint8_t*)symbols, symbol, sizeof(symbol) ); // reset symbols
}

static void beginAttract( void ) {
   resetVectors();
}


static void endAttract( void ) {
   resetVectors();

   // set font 'a' thru 'z' to regular white
   colorize( (uint8_t*)fontAddress('a'), fontAddress('z'+1)-fontAddress('a'), SEGA_COLOR_WHITE );

   // set numbers '0' thru '9' to pink
   colorize( (uint8_t*)fontAddress('0'), fontAddress('9'+1)-fontAddress('0'), SEGA_COLOR_MAGENTA );
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
         enableSymbol( S_LOGO, CENTER_X, CENTER_Y, SEGA_ANGLE(0), 0x80 );
         const char s[] = "game over";
         drawString( &symbols[S_STRING], CENTER_X-155, MIN_Y+40, 0x80, SEGA_COLOR_YELLOW, s, sizeof(s)-1 );
         last_tick = system_tick;
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
         state++;
         break;

      case 1: 
      case 3: {
         if ( system_tick - last_tick > SECONDS(3) ) {
            state_ix = 2;
            state_iy = CENTER_Y-250;
            state++;
         }
         break; }

      case 4:
      case 7:
      case 10: {
         symbols[S_LOGO].visible = false;
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


static inline bool checkColission( symbol_t *const s0, symbol_t *const s1 ) {
   uint16_t x0 = s0->x-(HITBOX_SZ/2);
   uint16_t x1 = s0->x+(HITBOX_SZ/2);
   uint16_t y0 = s0->y-(HITBOX_SZ/2);
   uint16_t y1 = s0->y+(HITBOX_SZ/2);
   return ( s1->x > x0 && s1->x < x1 && s1->y > y0 && s1->y < y1 );
}


static void beginPlay(void) {
   score = 0;
   resetSymbol( S_SCORE0, CENTER_X-40, MIN_Y+10, SEGA_ANGLE(0), 0x80 );
   resetSymbol( S_SCORE1, CENTER_X,    MIN_Y+10, SEGA_ANGLE(0), 0x80 );
   resetSymbol( S_SCORE2, CENTER_X+40, MIN_Y+10, SEGA_ANGLE(0), 0x80 );
   drawScore(score, true);
   spinner_vector_angle( true );
   enableSymbol( S_HAND, CENTER_X, CENTER_Y, 0, 0x40 );
   enableSymbol( S_HEAD, CENTER_X, CENTER_Y, SEGA_ANGLE(90), 0x40 );
   enableSymbol( S_SHIRT, CENTER_X+20, CENTER_Y-220, SEGA_ANGLE(90), 0x40 );
   enableSymbol( S_EXPRESSION1, CENTER_X+25, CENTER_Y-30, SEGA_ANGLE(90), 0x40 );
}


static bool drawPlay(void) {
   symbol_t *const ferengi = &symbols[S_HEAD];
   symbol_t *const hand = &symbols[S_HAND];

   uint16_t vec_angle = spinner_vector_angle( false );

   uint8_t buttons = PORT_374;

   static uint8_t ct=0;
   if ( ct == 0 ) {
      if ( buttons & BUTTON_FIRE ) {
         SOUND_COMMAND = TANK_FIRE;
      }
   }

   drawHand( vec_angle );

   return false;
}

static void beginGameOver(void) {
   if ( score <= high_score[2] ) {
      const char s[] = "game over";
      drawString( &symbols[S_STRING], CENTER_X-280, MIN_Y, 0xFE, SEGA_COLOR_RED, s, sizeof(s)-1 );
   }  else {
      const char s[] = "high score";
      drawString( &symbols[S_STRING], CENTER_X-280, MIN_Y, 0xFE, SEGA_COLOR_YELLOW, s, sizeof(s)-1 );
   }

   setTrajectory( S_STRING, 5, SEGA_ANGLE(0) );
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
               
            }
            break;

         case game_state_game_over_pause:
            if ( system_tick - last_tick > SECONDS(4) ) {
               game_state = game_state_boot;
            } else {
               
            }
            break;

         case game_state_highscore:
            if ( drawInitials() ) {
               game_state = game_state_boot;
            } else {
               
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

