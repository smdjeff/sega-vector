#include "sega.h"


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

extern symbol_t* sym_head;
extern symbol_t* sym_ear1;
extern symbol_t* sym_ear2;
extern symbol_t* sym_shirt;
extern symbol_t* sym_expression;
extern symbol_t* sym_hand1;
extern symbol_t* sym_hand2;
extern symbol_t* sym_latinum1;
extern symbol_t* sym_latinum2;
extern symbol_t* sym_latinum3;


typedef enum {
   HAPPY = 1, // sounds 1 ah, 3 yes
   ANGRY = 2, // no sound
   OHYES = 3  // sound 2 oomox
} expression_t;

static void drawExpression(vector_t* vec, expression_t ix) {
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

static void drawSparkle(uint16_t x, uint16_t y, uint16_t angle) {
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

static void drawHand(uint16_t vec_angle, uint8_t tickle) {
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


void ferengiGame( void ) {

   const char s1[] = "rub the lobes";
   // const char s2[] = "oomox for latinum bars";
   vec_string1 = ALLOC( measureString(s1) * sizeof(vector_t) );
   drawString( sym_string1, vec_string1, CENTER_X-460, MIN_Y+40, 0x40, SEGA_COLOR_YELLOW, s1 );
   // vec_string2 = ALLOC( measureString(s2) * sizeof(vector_t) );
   // drawString( sym_string2, vec_string2, CENTER_X-460, MIN_Y+40-40, 0x40, SEGA_COLOR_YELLOW, s2 );

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


   sym_string1->visible = false;
   // sym_string2->visible = false;
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
   FREE( vec_string1 );
   // FREE( vec_string2 );
   FREE( vec_latinum );
   FREE( vec_head );
   FREE( vec_ear1 );
   FREE( vec_ear2 );
   FREE( vec_shirt );
   FREE( vec_expression );
   FREE( vec_hand1 );
   FREE( vec_hand2 );
}
