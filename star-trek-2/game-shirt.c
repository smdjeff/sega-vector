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

static const uint16_t SHIRT_Y = CENTER_Y - 50;
static const int16_t SHIRT_X[3] = { CENTER_X-300, CENTER_X, CENTER_X+300 };
#define SHIRT_SPEED  40
#define ARC_SPEED    20

void shirtGame( void ) {

   vector_t* vec_shirt_yellow = ALLOC( sizeof(vector_shirt_yellow) );
   memcpy( vec_shirt_yellow, vector_shirt_yellow, sizeof(vector_shirt_yellow) );
   vector_t* vec_shirt_blue = ALLOC( sizeof(vector_shirt_blue) );
   memcpy( vec_shirt_blue, vector_shirt_blue, sizeof(vector_shirt_blue) );
   vector_t* vec_shirt_red = ALLOC( sizeof(vector_shirt_red) );
   memcpy( vec_shirt_red, vector_shirt_red, sizeof(vector_shirt_red) );

   symbol_t* slots[3] = { sym_shirt1, sym_shirt2, sym_shirt3 };

   drawSymbol( sym_shirt1, vec_shirt_yellow, SHIRT_X[0], SHIRT_Y, SEGA_ANGLE(0), 0x80 );
   drawSymbol( sym_shirt2, vec_shirt_blue,   SHIRT_X[1], SHIRT_Y, SEGA_ANGLE(0), 0x80 );
   drawSymbol( sym_shirt3, vec_shirt_red,    SHIRT_X[2], SHIRT_Y, SEGA_ANGLE(0), 0x80 );

   drawCountdown( 20, false );

   const char s1[] = "red shirt always dies";
   vec_string1 = ALLOC( measureString(s1) * sizeof(vector_t) );
   drawString( sym_string1, vec_string1, GAME_TEXT_X, GAME_TEXT_Y, GAME_TEXT_SIZE, SEGA_COLOR_YELLOW, s1 );
   delay(3000);
   sym_string1->visible = false;
   FREE( vec_string1 );

   drawCountdown( 20, false );

   while ( drawCountdown(0, false) ) {

      drawSymbol( sym_shirt1, vec_shirt_yellow, SHIRT_X[0], SHIRT_Y, SEGA_ANGLE(0), 0x80 );
      drawSymbol( sym_shirt2, vec_shirt_blue,   SHIRT_X[1], SHIRT_Y, SEGA_ANGLE(0), 0x80 );
      drawSymbol( sym_shirt3, vec_shirt_red,    SHIRT_X[2], SHIRT_Y, SEGA_ANGLE(0), 0x80 );

      // show box in blue during shuffling (button disabled)
      {
         uint16_t angle = spinner_vector_angle(false);
         int16_t pos = (angle > 511) ? (int16_t)angle - 1024 : (int16_t)angle;
         if (pos >  256) pos =  256;
         if (pos < -256) pos = -256;
         colorize( vec_box, 5, SEGA_COLOR_BLUE );
         drawSymbol( sym_box, vec_box, CENTER_X + (pos << 1), SHIRT_Y, SEGA_ANGLE(0), 0xff );
      }

      for (uint8_t i = 0; i < 4; i++) {

         // shuffle which shirt goes to which slot
         for (uint8_t j = 2; j > 0; j--) {
            uint8_t r = rand8() % (j + 1);
            symbol_t* temp = slots[j];
            slots[j] = slots[r];
            slots[r] = temp;
         }

         // compute move parameters for each shirt
         int8_t  vx[3], vy[3];
         int16_t mid_x[3];
         uint8_t arc_flipped[3];

         for (uint8_t s = 0; s < 3; s++) {
            int16_t cur_x = (int16_t)slots[s]->x;
            int16_t dx    = SHIRT_X[s] - cur_x;
            mid_x[s]       = cur_x + dx / 2;
            vx[s]          = (dx > 0) ? SHIRT_SPEED : (dx < 0) ? -SHIRT_SPEED : 0;
            vy[s]          = (dx != 0) ? ARC_SPEED : 0;
            arc_flipped[s] = (vx[s] == 0) ? 1 : 0;
            setSpeeds( SID(slots[s]), vx[s], vy[s] );
         }

         SOUND_COMMAND = PHASER;

         // animate each shirt to its target; stop it the moment it arrives
         uint8_t n_moving = 0;
         for (uint8_t s = 0; s < 3; s++) if (vx[s] != 0) n_moving++;

         while (n_moving > 0) {
            waitAnimate(1);
            for (uint8_t s = 0; s < 3; s++) {
               if (vx[s] == 0) continue;
               int16_t cur_x = (int16_t)slots[s]->x;
               // flip arc direction at the midpoint
               if ( !arc_flipped[s] ) {
                  bool past_mid = (vx[s] > 0) ? (cur_x >= mid_x[s]) : (cur_x <= mid_x[s]);
                  if ( past_mid ) {
                     arc_flipped[s] = 1;
                     setSpeeds( SID(slots[s]), vx[s], -vy[s] );
                  }
               }
               // snap and stop this shirt the moment it reaches its target
               if ( (vx[s] > 0 && cur_x >= SHIRT_X[s]) ||
                    (vx[s] < 0 && cur_x <= SHIRT_X[s]) ) {
                  slots[s]->x = SHIRT_X[s];
                  slots[s]->y = SHIRT_Y;
                  setStop( SID(slots[s]) );
                  vx[s] = 0;
                  n_moving--;
               }
            }
            // update box position during shuffle (button still disabled)
            {
               uint16_t angle = spinner_vector_angle(false);
               int16_t pos = (angle > 511) ? (int16_t)angle - 1024 : (int16_t)angle;
               if (pos >  256) pos =  256;
               if (pos < -256) pos = -256;
               sym_box->x = CENTER_X + (pos << 1);
            }
         }
      }

      // shuffle done — switch box to magenta for firing
      colorize( vec_box, 5, SEGA_COLOR_MAGENTA );

      symbol_t *sym_red_shirt = sym_shirt3;

      // timeout gets shorter as master countdown runs out
      uint8_t  cdown        = drawCountdown(0, false);
      uint16_t shirt_timeout = (SECONDS(1) >> 1) + ((uint16_t)cdown * cdown >> 1);
      uint16_t shirt_tick      = system_tick;
      uint16_t score_interval  = (shirt_timeout >> 3) - (shirt_timeout >> 6);  // ≈ shirt_timeout/9
      uint16_t next_decrement  = shirt_tick + score_interval;
      uint8_t  hit_score       = 10;
      uint8_t  diff_bonus      = (20 - cdown) >> 1;  // 0 at easy, 10 at hardest

      do {
         if (system_tick - shirt_tick > shirt_timeout) break; // reshuffle

         if (hit_score > 1 && system_tick >= next_decrement) {
            hit_score--;
            next_decrement += score_interval;
         }

         uint8_t buttons = PORT_374;
         if (buttons & BUTTON_FIRE) {
            if ( checkColission(sym_box, sym_shirt1) ) { setRotationSpeed( SID(sym_shirt1), 66); setResizeSpeed( SID(sym_shirt1), -15 ); }
            else if ( checkColission(sym_box, sym_shirt2) ) { setRotationSpeed( SID(sym_shirt2), 66); setResizeSpeed( SID(sym_shirt2), -15 ); }
            else if ( checkColission(sym_box, sym_shirt3) ) { setRotationSpeed( SID(sym_shirt3), 66); setResizeSpeed( SID(sym_shirt3), -15 ); }
            if ( checkColission(sym_box, sym_red_shirt) ) {
               uint8_t bonus = (system_tick - shirt_tick < (shirt_timeout >> 1)) ? diff_bonus : 0;
               score += hit_score + bonus;
               drawScore(score,false);
               SOUND_COMMAND = STARBASE_RED;
            } else {
               SOUND_COMMAND = DOCK;
               delay( SECONDS(0.33) ); // extra penalty for missing
            }
            waitAnimate( SECONDS(0.25) );
            setStop( SID(sym_shirt1) );
            setStop( SID(sym_shirt2) );
            setStop( SID(sym_shirt3) );
            break;
         }
         uint16_t angle = spinner_vector_angle( false );
         int16_t pos = (angle > 511) ? (int16_t)angle - 1024 : (int16_t)angle;
         if (pos >  256) pos =  256;
         if (pos < -256) pos = -256;
         drawSymbol( sym_box, vec_box, CENTER_X + (pos << 1), SHIRT_Y, SEGA_ANGLE(0), 0xff );
      } while ( drawCountdown(0,false) );

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
