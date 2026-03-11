
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
static uint16_t score = 0;
static uint16_t high_score[3] = { 789, 456, 123 };
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

   // static uint16_t last_tick = 0;
   // // while ( system_tick - last_tick < 750/25 ) {
   // //    // wait for speech to finish, sending too fast causes glitches
   // //    // but unclear how to receive interrupt on complete of phrase
   // //    // so just busy wait
   // //    //SPEECH_COMMAND = NO_PHRASE;
   // //    __asm__( "nop" );
   // // }
//   writeDebug('s',i);
   SPEECH_COMMAND = 0x00; // stop existing playback
   SPEECH_COMMAND = i;
   SPEECH_COMMAND = i | 0x80;

   // uint16_t last_tick = system_tick;
   // while ( system_tick - last_tick < 750/25 ) {
   //    // wait for speech to finish, sending too fast causes glitches
   //    // but unclear how to receive interrupt on complete of phrase
   //    // so just busy wait
   //    //SPEECH_COMMAND = NO_PHRASE;
   //    __asm__( "nop" );
   //    uint8_t r = SPEECH_COMMAND;
   //    writeDebug('S',r);
   // }

}


/////// attract sequence

const uint8_t vector_logo[] = {
    #include "art/logo.h"
};

///////

/////// game 1

const uint8_t vector_latinum[] = {
   #include "art/latinum.h"
};

const uint8_t vector_head[] = {
    #include "art/head.h"
};

const uint8_t vector_ear1[] = {
    #include "art/ear1.h"
};

const uint8_t vector_ear2[] = {
    #include "art/ear2.h"
};

const uint8_t vector_shirt[] = {
    #include "art/shirt.h"
};

const uint8_t vector_hand1[] = {
    #include "art/hand1.h"
};

const uint8_t vector_hand2[] = {
    #include "art/hand2.h"
};

const uint8_t vector_expression1[] = {
    #include "art/expression1.h"
};

const uint8_t vector_expression2[] = {
    #include "art/expression2.h"
};

const uint8_t vector_expression3[] = {
    #include "art/expression3.h"
};

////////

/////// game 2

const uint8_t vector_shirt_yellow[] = {
    #include "art/shirt-yellow.h"
};

const uint8_t vector_shirt_blue[] = {
    #include "art/shirt-blue.h"
};

const uint8_t vector_shirt_red[] = {
    #include "art/shirt-red.h"
};

//////// game 3

const uint8_t vector_jar[] = {
    #include "art/jar.h"
};

const uint8_t vector_crystal1[] = {
    #include "art/crystal1.h"
};

const uint8_t vector_crystal2[] = {
    #include "art/crystal2.h"
};

const uint8_t vector_crystal3[] = {
    #include "art/crystal3.h"
};

////////

// cliché-based mini-game ideas
// funny and dub could be inspiring for middle finger to the Trek license
//  Or BishiBashi. Lolz on the burrito and toilet games at 1:45 and 20:00
// https://youtu.be/OdYCigrFMSQ?si=89AgwZ54QfINq8Yr

// * Rub the Ferengi ear.
// * Red Shirt guy always dies - like a bishibashi
// * Scotty fixing dilithium crystal matrix
// other ideas:
// 1. Captain Pike yes/no button.
// 2. Tribbles - like Tron spiders.
// 3. Dr McCoy Tricorder I'm a doctor not a miracle worker.


vector_t* vectors     = NULL;
vector_t* vec_line    = NULL;
vector_t* vec_box     = NULL;
vector_t* vec_score   = NULL;
vector_t* vec_counter = NULL;
vector_t* vec_logo    = NULL;
vector_t* vec_string1 = NULL;
vector_t* vec_string2 = NULL;
vector_t* vec_string3 = NULL;

uint8_t* heap;

void initVectors(void) {

   if ( vectors ) {
      heap_free( heap, vectors );
   }
   vectors = heap_alloc( heap, sizeof(vector_t)*(2+5) );
   writeDebug('e',vectors);
   if ( !vectors ) kill( 0x01 );

   vector_t* vec = vectors;

   vec_line = vec;
   initVector( vec, 0xFF, SEGA_ANGLE(0), SEGA_COLOR_YELLOW|SEGA_LAST ); vec++;

   vec_box = vec;
   initVector( vec, (80/1.4), SEGA_ANGLE(225), SEGA_CLEAR ); vec++;
   initVector( vec, (80),     SEGA_ANGLE(0),   SEGA_COLOR_MAGENTA ); vec++;
   initVector( vec, (80),     SEGA_ANGLE(90),  SEGA_COLOR_MAGENTA ); vec++;
   initVector( vec, (80),     SEGA_ANGLE(180), SEGA_COLOR_MAGENTA ); vec++;
   initVector( vec, (80),     SEGA_ANGLE(270), SEGA_COLOR_MAGENTA|SEGA_LAST );
}

   
symbol_t *const symbols = (symbol_t*)(VECTOR_RAM); // must be at the top of vector ram

uint8_t symbols_count;
symbol_t* sym_score;
symbol_t* sym_counter;
symbol_t* sym_string1;
symbol_t* sym_string2;
symbol_t* sym_string3;
symbol_t* sym_logo;

symbol_t* sym_box;
symbol_t* sym_shirt1;
symbol_t* sym_shirt2;
symbol_t* sym_shirt3;

symbol_t* sym_head;
symbol_t* sym_ear1;
symbol_t* sym_ear2;
symbol_t* sym_shirt;
symbol_t* sym_expression;
symbol_t* sym_hand1;
symbol_t* sym_hand2;
symbol_t* sym_latinum1;
symbol_t* sym_latinum2;
symbol_t* sym_latinum3;

symbol_t* sym_jar;
symbol_t* sym_crystal1;
symbol_t* sym_crystal2;
symbol_t* sym_crystal3;

void initSymbols(void) {

   symbols_count  = 0;
   sym_score      = &symbols[symbols_count++];
   sym_counter    = &symbols[symbols_count++];
   sym_string1    = &symbols[symbols_count++];
   sym_string2    = &symbols[symbols_count++];
   sym_string3    = &symbols[symbols_count++];
   sym_logo       = &symbols[symbols_count++];

   sym_box        = &symbols[symbols_count++];
   sym_shirt1     = &symbols[symbols_count++];
   sym_shirt2     = &symbols[symbols_count++];
   sym_shirt3     = &symbols[symbols_count++];

   sym_head       = &symbols[symbols_count++];
   sym_ear1       = &symbols[symbols_count++];
   sym_ear2       = &symbols[symbols_count++];
   sym_shirt      = &symbols[symbols_count++];
   sym_expression = &symbols[symbols_count++];
   sym_hand1      = &symbols[symbols_count++];
   sym_hand2      = &symbols[symbols_count++];
   sym_latinum1   = &symbols[symbols_count++];
   sym_latinum2   = &symbols[symbols_count++];
   sym_latinum3   = &symbols[symbols_count++];

   sym_jar        = &symbols[symbols_count++];
   sym_crystal1   = &symbols[symbols_count++];
   sym_crystal2   = &symbols[symbols_count++];
   sym_crystal3   = &symbols[symbols_count++];

   symbol_t* const end = symbols + symbols_count;
   for (symbol_t* sym = symbols; sym < end; sym++) {
      initSymbol(sym, vec_box);
   }
   symbols[symbols_count-1].last = true;

   heap = (uint8_t*)&symbols[symbols_count];
   uint16_t used = (uint16_t)heap - VECTOR_RAM;
   heap_init( heap, VECTOR_RAM_SZ - used );

   initVectors();

   writeDebug('s', symbols);
   writeDebug('h', heap);
   writeDebug('v', vectors);
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

static void beginAttract( void ) {
   resetAnimate();

   vec_logo = ALLOC( sizeof(vector_logo) );
   memcpy( vec_logo, vector_logo, sizeof(vector_logo) );
}

static void endAttract( void ) {
   sym_logo->visible = false;
   sym_string1->visible = false;
   sym_string2->visible = false;
   sym_string3->visible = false;
   FREE( vec_logo );
   FREE( vec_string1 );
   FREE( vec_string2 );
   FREE( vec_string3 );
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
      // jumping states means flushing everything
      state_ix = 0;
      state_iy = 0;
      last_tick = 0;
      endAttract();
      beginAttract();
      drawSymbol( sym_logo, vec_logo, CENTER_X, CENTER_Y, SEGA_ANGLE(0), 0xC0 );
      state = 2;
   }

   switch ( state ) {

      case 0: {
         // leak test (should not go up)
         void *x = heap_alloc( heap, sizeof(vector_t) );
         writeDebug('h',(uint16_t)x);
         heap_free( heap, x );

         drawSymbol( sym_logo, vec_logo, CENTER_X, CENTER_Y, SEGA_ANGLE(0), 1 );
         setResizeSpeed( SID(sym_logo), 1 );
         const char s[] = "game over";
         vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );
         drawString( sym_string1, vec_string1, CENTER_X-100, MIN_Y+40, 0x60, SEGA_COLOR_WHITE, s );
         last_tick = system_tick;
         state++;
         break; }
      
      case 1:
         if ( system_tick - last_tick > SECONDS(3) ) {
            sym_string1->visible = false;
            FREE( vec_string1 );
            state++;
         }
         break;

      case 2: {
         char s[12] = {0,};
         uint8_t color = 0;
         if ( _coin_counter > 0 ) {
            memcpy( s, "press start", 11 );
            color = SEGA_COLOR_GREEN;
         } else {
            memcpy( s, "insert coin", 11 );
            color = SEGA_COLOR_CYAN;
         }
         vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );
         drawString( sym_string1, vec_string1, CENTER_X-120, MIN_Y+40, 0x60, color, s );
         last_tick = system_tick;
         state++;
         break; }

      case 3:
         if ( system_tick - last_tick > SECONDS(3) ) {
            sym_string1->visible = false;
            FREE( vec_string1 );            
            state++;
         }
         break;

      case 4: {
         sym_logo->visible = false;
         switch ( state_ix ) {
            case 0: {
               const char s[] = "lieutenant";
               vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );         
               drawString( sym_string1, vec_string1, CENTER_X-220, CENTER_Y+100, 0xA0, SEGA_COLOR_MAGENTA, s );
               break; }
            case 1: {
               const char s[] = "commander";
               vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );         
               drawString( sym_string1, vec_string1, CENTER_X-200, CENTER_Y+100, 0xA0, SEGA_COLOR_MAGENTA, s );
               break; }
            case 2: {
               const char s[] = "captain";
               vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );         
               drawString( sym_string1, vec_string1, CENTER_X-150, CENTER_Y+100, 0xA0, SEGA_COLOR_MAGENTA, s );
               break; }
         }
         char s[9] = "abc 1234";
         memcpy( &s[0], high_name[2-state_ix], 3 );
         dec4( &s[4], high_score[2-state_ix] );
         vec_string2 = ALLOC( measureString(s) * sizeof(vector_t) );         
         drawString( sym_string2, vec_string2, MAX_X, CENTER_Y-100, 0x80, SEGA_COLOR_CYAN, s );
         setTrajectory( SID(sym_string2), 20, SEGA_ANGLE(270) );
         state++;
         break; }

      case 5:
         if ( sym_string2->x < CENTER_X-150 ) {
            setStop( SID(sym_string2) );
            last_tick = system_tick;         
            state++;
         }
         break;

      case 6:
         if ( system_tick - last_tick > SECONDS(1.5) ) {
            sym_string1->visible = false;
            sym_string2->visible = false;
            FREE( vec_string1 );
            FREE( vec_string2 );
            state_ix ++;
            if ( state_ix < 3 ) {
               state-=2;
            } else {
               state_ix = 0;
               state = 0;
            }
         }
         break;

      default:
         state = 0;
         break;
    }

   return false;
}


static void crystalGame( void ) {
   vector_t* vec_jar = ALLOC( sizeof(vector_jar) );
   memcpy( vec_jar, vector_jar, sizeof(vector_jar) );
   vector_t* vec_crystal1 = ALLOC( sizeof(vector_crystal1) );
   memcpy( vec_crystal1, vector_crystal1, sizeof(vector_crystal1) );
   vector_t* vec_crystal2 = ALLOC( sizeof(vector_crystal2) );
   memcpy( vec_crystal2, vector_crystal2, sizeof(vector_crystal2) );
   vector_t* vec_crystal3 = ALLOC( sizeof(vector_crystal3) );
   memcpy( vec_crystal3, vector_crystal3, sizeof(vector_crystal3) );

   drawSymbol( sym_jar, vec_jar, CENTER_X, CENTER_Y, SEGA_ANGLE(0), 0xE0 );
   drawSymbol( sym_crystal1, vec_crystal1, CENTER_X-120, CENTER_Y, SEGA_ANGLE(0), 0x60 );
   drawSymbol( sym_crystal2, vec_crystal2, CENTER_X,     CENTER_Y, SEGA_ANGLE(0), 0x60 );
   drawSymbol( sym_crystal3, vec_crystal3, CENTER_X+120, CENTER_Y, SEGA_ANGLE(0), 0x60 );

   symbol_t* sym_list[] = { sym_crystal1, sym_crystal2, sym_crystal3 };

   say( POWER_FAILED );
   delay( 1000 );

   SOUND_COMMAND = NOMAD_MOTION;

   for (uint16_t i = 0; i < 500; i++) {
      for (uint8_t ix = 0; ix < 3; ix++) {
         symbol_t* sym = sym_list[ix];
         uint8_t r = rand8();
         if (r & 0x01) {
            sym->rotation += SEGA_ANGLE(2);
         } else {
            sym->rotation -= SEGA_ANGLE(2);
         }
         if (r & 0x02) {
            sym->x += 2;
         } else {
            sym->x -= 2;
         }
         if (r & 0x04) {
            sym->y += 2;
         } else {
            sym->y -= 2;
         }
         if (r & 0x08) {
            sym->scale += 2;
         } else {
            sym->scale -= 2;
         }
      }
      delay(10);
   }

   SOUND_COMMAND = NOMAD_MOTION_END;

   say( POWER_RESTORED );
   delay(1000);

   sym_jar->visible = false;
   sym_crystal1->visible = false;
   sym_crystal2->visible = false;
   sym_crystal3->visible = false;
   FREE( vec_jar );
   FREE( vec_crystal1 );
   FREE( vec_crystal2 );
   FREE( vec_crystal3 );
}

static void waitAnimate( uint16_t ticks ) {
   uint16_t last_tick = system_tick;
   do {
      animate( system_tick, symbols, symbols_count );
   } while ( system_tick - last_tick < ticks );
}

static const int16_t SHIRT_X[3] = { CENTER_X-300, CENTER_X, CENTER_X+300 };
#define SHIRT_MOVE_TICKS  10
#define ARC_SPEED         10

static void shirtGame( void ) {
   vector_t* vec_shirt_yellow = ALLOC( sizeof(vector_shirt_yellow) );
   memcpy( vec_shirt_yellow, vector_shirt_yellow, sizeof(vector_shirt_yellow) );
   vector_t* vec_shirt_blue = ALLOC( sizeof(vector_shirt_blue) );
   memcpy( vec_shirt_blue, vector_shirt_blue, sizeof(vector_shirt_blue) );
   vector_t* vec_shirt_red = ALLOC( sizeof(vector_shirt_red) );
   memcpy( vec_shirt_red, vector_shirt_red, sizeof(vector_shirt_red) );


   symbol_t* slots[3] = { sym_shirt1, sym_shirt2, sym_shirt3 };

   drawCountdown( 20 );
   while ( drawCountdown(0) ) {

      sym_box->visible = false;

      drawSymbol( sym_shirt1, vec_shirt_yellow, SHIRT_X[0], CENTER_Y, SEGA_ANGLE(0), 0x80 );
      drawSymbol( sym_shirt2, vec_shirt_blue,   SHIRT_X[1], CENTER_Y, SEGA_ANGLE(0), 0x80 );
      drawSymbol( sym_shirt3, vec_shirt_red,    SHIRT_X[2], CENTER_Y, SEGA_ANGLE(0), 0x80 );

      for (uint8_t i=0; i<4; i++) {
         int8_t arc[3];
         int8_t vx_saved[3];

         for (uint8_t j = 2; j > 0; j--) {
            uint8_t r = rand8() % (j + 1);
            symbol_t* temp = slots[j];
            slots[j] = slots[r];
            slots[r] = temp;
         }

         for (uint8_t s = 0; s < 3; s++) {
            symbol_t* sym = slots[s];
            int16_t dx  = SHIRT_X[s] - sym->x;
            vx_saved[s] = dx / SHIRT_MOVE_TICKS;
            arc[s]      = (dx > 0) ? -ARC_SPEED : (dx < 0) ? ARC_SPEED : 0;
            setSpeeds( SID(sym), vx_saved[s], arc[s] );
         }

         SOUND_COMMAND = PHASER;
         waitAnimate( SECONDS(0.125) );

         for (uint8_t s = 0; s < 3; s++) {
            setSpeeds( SID(slots[s]), vx_saved[s], -arc[s] );
         }

         waitAnimate( SECONDS(0.125) );

         for (uint8_t s = 0; s < 3; s++) {
            slots[s]->x = SHIRT_X[s];
            slots[s]->y = CENTER_Y;
            setStop( SID(slots[s]) );
         }
      }

      symbol_t *sym_red_shirt = sym_shirt3;

      do {
         uint8_t buttons = PORT_374;
         if (buttons & BUTTON_FIRE) {
            if ( checkColission(sym_box, sym_shirt1) ) { setRotationSpeed( SID(sym_shirt1), 66); setResizeSpeed( SID(sym_shirt1), -15 ); }
            else if ( checkColission(sym_box, sym_shirt2) ) { setRotationSpeed( SID(sym_shirt2), 66); setResizeSpeed( SID(sym_shirt2), -15 ); }
            else if ( checkColission(sym_box, sym_shirt3) ) { setRotationSpeed( SID(sym_shirt3), 66); setResizeSpeed( SID(sym_shirt3), -15 ); }
            if ( checkColission(sym_box, sym_red_shirt) ) {
               score++;
               drawScore(score,false);
               SOUND_COMMAND = STARBASE_RED;
            } else {
               SOUND_COMMAND = DOCK;
            }
            waitAnimate( SECONDS(0.75) );
            setStop( SID(sym_shirt1) );
            setStop( SID(sym_shirt2) );
            setStop( SID(sym_shirt3) );
            break;
         }
         uint16_t angle = spinner_vector_angle( false );
         int16_t dx, dy;
         vectorToXY(angle, 500, &dx, &dy);
         drawSymbol( sym_box, vec_box, CENTER_X+dx, CENTER_Y, SEGA_ANGLE(0), 0xff );
      } while ( drawCountdown(0) );

      sym_box->visible = false;

   }

   sym_shirt1->visible = false;
   sym_shirt2->visible = false;
   sym_shirt3->visible = false;
   FREE( vec_shirt_yellow );
   FREE( vec_shirt_blue );
   FREE( vec_shirt_red );
}

typedef enum {
   HAPPY = 1, // sounds 1 ah, 3 yes
   ANGRY = 2, // no sound
   OHYES = 3  // sound 2 oomox
} expression_t;

void drawExpression(vector_t* vec, expression_t ix) {
   static expression_t last_expression = 0;
   if (ix != last_expression) {
      switch (ix) {
      case HAPPY:
         if ( rand8() & 0x01 ) say(FERENGI_AH); else say(FERENGI_YES);
         sym_expression->visible = false;
         memcpy( vec, vector_expression1, sizeof(vector_expression1) );
         sym_expression->visible = true; 
         break;
      case ANGRY:
         sym_expression->visible = false;
         memcpy( vec, vector_expression2, sizeof(vector_expression2) );
         sym_expression->visible = true; 
         break;
      case OHYES:
         sym_expression->visible = false;
         memcpy( vec, vector_expression3, sizeof(vector_expression3) );
         sym_expression->visible = true; 
         break;
      default: 
         sym_expression->visible = false;
         break;
      }
      last_expression = ix;
   }
}

void drawSparkle(uint16_t x, uint16_t y, uint16_t angle) {
   symbol_t* sym = 0;
   if (!sym_latinum1->visible) sym=sym_latinum1;
   else if (!sym_latinum2->visible) sym=sym_latinum2;
   else if (!sym_latinum3->visible) sym=sym_latinum3;

   if (sym) {
      enableSymbol( sym, x, y, SEGA_ANGLE(0), 0x60 );
      setTrajectory( SID(sym), 5, angle );
      setResizeSpeed( SID(sym), -3 );
      setRotationSpeed( SID(sym), 20 + (rand8() & 0x3F) );
   }
}

void drawHand(uint16_t vec_angle, uint8_t tickle) {
   int16_t dx, dy, finger_x;
   vectorToXY(vec_angle, 240, &dx, &dy);

   dy = divide1_5( dy );

   sym_hand1->visible = (dx > 0);
   sym_hand2->visible = (dx < 0);

   sym_hand1->x = CENTER_X - 0 + dx;
   sym_hand1->y = CENTER_Y + 50 - dy;
   sym_hand2->x = CENTER_X + 0 + dx;
   sym_hand2->y = CENTER_Y + 70 - dy;

   sym_hand1->rotation = (SEGA_ANGLE(0) + (dy >> 0)) & 0x03FF;
   sym_hand2->rotation = (SEGA_ANGLE(0) - (dy >> 0)) & 0x03FF;

   if (dy < 0) {
      finger_x = sym_hand2->rotation;
   } else {
      finger_x = -((0x3FF - sym_hand2->rotation) >> 2); 
   }

   if ( tickle == 1 ) {
      // left ear
      uint16_t x = (int16_t)sym_hand1->x - finger_x; 
      uint16_t y = sym_hand1->y + 50;
      drawSparkle( x, y, SEGA_ANGLE(225)+rand8());
   }

   if ( tickle == 2 ) {
      // right ear
      uint16_t x = (int16_t)sym_hand2->x + 10 + finger_x;
      uint16_t y = sym_hand2->y + 50;
      drawSparkle( x, y, SEGA_ANGLE(45)+rand8() );
   }
}

static void ferengiGame( void ) {

   vector_t* vec_latinum = ALLOC( sizeof(vector_latinum) );
   memcpy( vec_latinum, vector_latinum, sizeof(vector_latinum) );
   initSymbol( sym_latinum1, vec_latinum );
   initSymbol( sym_latinum2, vec_latinum );
   initSymbol( sym_latinum3, vec_latinum );

   vector_t* vec_head = ALLOC( sizeof(vector_head) );
   memcpy( vec_head, vector_head, sizeof(vector_head) );
   drawSymbol( sym_head, vec_head, CENTER_X, CENTER_Y+100, SEGA_ANGLE(0), 0x80 );

   vector_t* vec_ear1 = ALLOC( sizeof(vector_ear1) );
   memcpy( vec_ear1, vector_ear1, sizeof(vector_ear1) );
   drawSymbol( sym_ear1, vec_ear1, CENTER_X+188, CENTER_Y+78, SEGA_ANGLE(0), 0x80 );

   vector_t* vec_ear2 = ALLOC( sizeof(vector_ear2) );
   memcpy( vec_ear2, vector_ear2, sizeof(vector_ear2) );
   drawSymbol( sym_ear2, vec_ear2, CENTER_X-174, CENTER_Y+111, SEGA_ANGLE(0), 0x80 );

   vector_t* vec_shirt = ALLOC( sizeof(vector_shirt) );
   memcpy( vec_shirt, vector_shirt, sizeof(vector_shirt) );
   drawSymbol( sym_shirt, vec_shirt, CENTER_X+10, CENTER_Y-160, SEGA_ANGLE(0), 0x80 );

   vector_t* vec_expression = ALLOC( MAX(MAX(sizeof(vector_expression1),sizeof(vector_expression2)),sizeof(vector_expression3)) );
   memcpy( vec_expression, vector_expression2, sizeof(vector_expression2) );
   drawSymbol( sym_expression, vec_expression, CENTER_X+20, CENTER_Y+70, SEGA_ANGLE(0), 0x80 );

   vector_t* vec_hand1 = ALLOC( sizeof(vector_hand1) );
   memcpy( vec_hand1, vector_hand1, sizeof(vector_hand1) );
   initSymbol( sym_hand1, vec_hand1 );
   vector_t* vec_hand2 = ALLOC( sizeof(vector_hand2) );
   memcpy( vec_hand2, vector_hand2, sizeof(vector_hand2) );
   initSymbol( sym_hand2, vec_hand2 );

   uint16_t last_spawn_angle = 0;
   uint16_t last_angle = 0;
   symbol_t* lat_symbols[] = {sym_latinum1, sym_latinum2, sym_latinum3};

   spinner_vector_angle( true );
   drawCountdown(20);

   uint16_t ear_tick = system_tick - SECONDS(3);
   bool active_right_ear = false;
   uint16_t tickle_angle = 0;
   uint8_t tickle = 0;

   while ( drawCountdown(0) ) {
  
      waitAnimate(0);
      drawScore( score, false );

      if ( system_tick - ear_tick > SECONDS(3) ) {
         ear_tick = system_tick;

         active_right_ear = rand8() & 0x01;
         if ( active_right_ear ) {
            colorize( vec_ear1, sizeof(vector_ear1)/sizeof(vector_t), SEGA_COLOR_MAGENTA );
            colorize( vec_ear2, sizeof(vector_ear2)/sizeof(vector_t), SEGA_COLOR_ORANGE );
         } else {
            colorize( vec_ear1, sizeof(vector_ear1)/sizeof(vector_t), SEGA_COLOR_ORANGE );
            colorize( vec_ear2, sizeof(vector_ear2)/sizeof(vector_t), SEGA_COLOR_MAGENTA );
         }
      }

      uint16_t angle = spinner_vector_angle( false );
      static uint16_t l_angle = 0xffff;
      if (angle == l_angle) continue;
      l_angle = angle;

      bool in_right_ear = (angle > SEGA_ANGLE(49) && angle < SEGA_ANGLE(102));
      bool in_left_ear  = (angle > SEGA_ANGLE(260) && angle < SEGA_ANGLE(317));

      tickle = 0;
      if (angle > tickle_angle + SEGA_ANGLE(15) || 
          angle < tickle_angle - SEGA_ANGLE(15) ) {
         tickle_angle = angle;
         if (in_left_ear && !active_right_ear) tickle = 1;
         if (in_right_ear && active_right_ear) tickle = 2;
      }

      drawHand( angle, tickle );

      if ( tickle ) {
         score++;
         drawExpression( vec_expression, HAPPY );
      } else {
         if ( angle == tickle_angle ) {
            drawExpression( vec_expression, ANGRY );
         }
      }

   }


   sym_latinum1->visible = false;
   sym_latinum2->visible = false;
   sym_latinum3->visible = false;
   sym_hand1->visible = false;
   sym_hand2->visible = false;
   colorize( vec_ear1, sizeof(vector_ear1)/sizeof(vector_t), SEGA_COLOR_ORANGE );
   colorize( vec_ear2, sizeof(vector_ear2)/sizeof(vector_t), SEGA_COLOR_ORANGE );

   drawExpression( vec_expression, HAPPY );
   delay(1000);
   say(FERENGI_OOMOX);
   delay(6000);

   sym_head->visible = false;
   sym_ear1->visible = false;
   sym_ear2->visible = false;
   sym_shirt->visible = false;
   sym_expression->visible = false;
   FREE( vec_latinum );
   FREE( vec_head );
   FREE( vec_ear1 );
   FREE( vec_ear2 );
   FREE( vec_shirt );
   FREE( vec_expression );
   FREE( vec_hand1 );
   FREE( vec_hand2 );
}


static uint8_t high_index = 0;
static uint8_t initials_ix = 0;
static char initials_last_ch = 0;
static char initials[4] = {0,};
static uint16_t vec_length = 0;

static inline void redrawString( void ) {
   drawString( sym_string3, vec_string3, CENTER_X-70, CENTER_Y-220, 0xF0, SEGA_COLOR_WHITE, initials );
}

static void beginDrawInitials( void ) {
   // say( HIGH_SCORE );
   initials_ix = 0;
   initials_last_ch = 0;
   memcpy( initials, "   ", 3 );
   const char s[] = "888";
   vec_length = measureString(s) * sizeof(vector_t);
   vec_string3 = ALLOC( vec_length );
   redrawString();
   spinner_vector_angle( true );

   if ( score >= high_score[0] ) {
      high_index = 0;
      high_score[2] = high_score[1];
      high_score[1] = high_score[0];
      high_score[0] = score;
      memcpy( high_name[2], high_name[1], 3 );
      memcpy( high_name[1], high_name[0], 3 );
      memset( high_name[0], 0x00, 3 );
      return;
   }
   if ( score >= high_score[1] ) {
      high_index = 1;
      high_score[2] = high_score[1];
      high_score[1] = score;
      memcpy( high_name[2], high_name[1], 3 );
      memset( high_name[1], 0x00, 3 );
      return;
   }
   if ( score >= high_score[2] ) {
      high_index = 2;
      high_score[2] = score;
      memset( high_name[2], 0x00, 3 );
      return;
   }
}


static bool drawInitials( void ) {
   uint16_t vec_angle = spinner_vector_angle( false );
   char ch = 'a' + div_16( vec_angle, 39 );
   ch = MIN( MAX(ch, 'a'), 'z' );

   if ( ch != initials_last_ch ) {
      initials_last_ch = ch;
      initials[initials_ix] = ch;
      redrawString();
   } else {
      static uint16_t last_tick = 0;
      if ( (system_tick - last_tick) > 6 ) {
         last_tick = system_tick;
         initials[initials_ix] = (initials[initials_ix] == ' ') ? ch : ' ';
         redrawString();
      }
   }

   static uint16_t last_button_tick = 0;
   uint8_t buttons = PORT_374;
   if ((buttons & BUTTON_FIRE) && ((system_tick - last_button_tick) > 33)) {
      last_button_tick = system_tick;
      initials[initials_ix] = ch;
      high_name[high_index][initials_ix] = ch;
      initials_ix++;
      redrawString();
      if (initials_ix == 3) {
//         say( CONGRATULATIONS );
         for (uint8_t color = 0; color < 0x3F; color++) {
            colorize( vec_string3, vec_length, color );
            static uint16_t lt = 0;
            while ( system_tick == lt ) { __asm__( "nop" ); }
            lt = system_tick;
         }
         sym_string3->visible = false;
         FREE( vec_string3 );
         return true;
      }
   }
   return false;
}

void drawScore( uint16_t score, bool reset ) {
   static uint16_t last_score = 0;
   if ( !vec_score ) {
      const char s[] = "8888";
      vec_score = ALLOC( measureString(s) * sizeof(vector_t) );
   }
   if ( reset || score != last_score ) {
      last_score = score;
      char s[5] = {0,};
      sym_score->visible = false;
      dec4( s, score );
      drawString( sym_score, vec_score, CENTER_X-40, MAX_Y-60, 0x60, SEGA_COLOR_WHITE, s );
   }
}

uint8_t drawCountdown( uint8_t initValue ) {
   static uint16_t last_tick = 0;
   static uint8_t value = 0;
   if ( initValue ) {
      last_tick = system_tick;
      value = initValue;
      if ( !vec_counter ) {
         const char s[] = "88";
         vec_counter = ALLOC( measureString(s) * sizeof(vector_t) );
      }
   } 
   if ( system_tick - last_tick > 40 ) {
      last_tick = system_tick;
      if (value) value--;
      char s[3] = {0,};
      dec2( s, value );
      drawString( sym_counter, vec_counter, CENTER_X-480, CENTER_Y+200, 0xF0, SEGA_COLOR_YELLOW, s );
      if ( value == 5 ) say( COUNT_5 );
      else if ( value == 4 ) say( COUNT_4 );
      else if ( value == 3 ) say( COUNT_3 );
      else if ( value == 2 ) say( COUNT_2 );
      else {
         SOUND_COMMAND = SAUCER_EXIT;
         delay(25);
         SOUND_COMMAND = SAUCER_EXIT_END;
      }
   }
   return value;
}

static void beginPlay(void) {
   score = 0;
   drawScore(score, true);
   spinner_vector_angle( true );
}


static bool drawPlay(void) {

   shirtGame();

   ferengiGame();

   crystalGame();

   return true;
}

static void beginGameOver(void) {
   if ( score <= high_score[2] ) {
      const char s[] = "game over";
      vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );
      drawString( sym_string1, vec_string1, CENTER_X-280, MIN_Y, 0xF0, SEGA_COLOR_RED, s );
   }  else {
      const char s[] = "high score";
      vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );
      drawString( sym_string1, vec_string1, CENTER_X-280, MIN_Y, 0xF0, SEGA_COLOR_YELLOW, s );
   }

   setTrajectory( SID(sym_string1), 5, SEGA_ANGLE(0) );
}

static bool drawGameOver(void) {
   // wait for text to slide into place
   if ( sym_string1->y > MAX_Y - 200 ) {
      setStop( SID(sym_string1) );
      return true;
   }
   return false;
}

static void endGameOver(void) {
   sym_string1->visible = false;
   FREE( vec_string1 );
}

static void super_loop(void) {
      static uint16_t last_tick = 0;

      animate( system_tick, symbols, symbols_count );

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
                  endGameOver();
               } else {
                  game_state = game_state_highscore;
                  beginDrawInitials();
               }
            } else {
               
            }
            break;

         case game_state_game_over_pause:
            if ( system_tick - last_tick > SECONDS(4) ) {
               endGameOver();
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

         // case game_state_diagnostics_io_init:
         //    beginDiagnosticsIO();
         //    game_state = game_state_diagnostics_io;
         //    break;

         // case game_state_diagnostics_io:
         //    drawDiagnosticsIO();
         //    break;

         // case game_state_diagnostics_grid_init:
         //    beginDiagnosticsGrid();
         //    game_state = game_state_diagnostics_grid;
         //    break;

         // case game_state_diagnostics_grid:
         //    drawDiagnosticsGrid();
         //    break;

      }
}



static void init(void) {

   SPEECH_CONTROL = 0x28;
   // SPEECH_COMMAND = 0x00;
   // SPEECH_COMMAND = 0x80;
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

   initSymbols();

   __asm__("ei");
   __asm__("halt");

   for (;;) {
      super_loop();
   }

}

