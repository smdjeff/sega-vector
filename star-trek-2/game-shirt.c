#include "sega.h"


const uint8_t vector_shirt_yellow[] = {
    #include "art/shirt-yellow.h"
};

const uint8_t vector_shirt_blue[] = {
    #include "art/shirt-blue.h"
};

const uint8_t vector_shirt_red[] = {
    #include "art/shirt-red.h"
};

extern symbol_t* sym_box;
extern symbol_t* sym_shirt1;
extern symbol_t* sym_shirt2;
extern symbol_t* sym_shirt3;

static const int16_t SHIRT_X[3] = { CENTER_X-300, CENTER_X, CENTER_X+300 };
#define SHIRT_MOVE_TICKS  10
#define ARC_SPEED         10

void shirtGame( void ) {

   const char s1[] = "red shirt guy always dies";
   vec_string1 = ALLOC( measureString(s1) * sizeof(vector_t) );
   drawString( sym_string1, vec_string1, CENTER_X-460, MIN_Y+40, 0x40, SEGA_COLOR_YELLOW, s1 );

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

   sym_string1->visible = false;
   sym_shirt1->visible = false;
   sym_shirt2->visible = false;
   sym_shirt3->visible = false;
   FREE( vec_string1 );
   FREE( vec_shirt_yellow );
   FREE( vec_shirt_blue );
   FREE( vec_shirt_red );
}
