#include "sega.h"


const uint8_t vector_sparkle[] = {
   #include "art/sparkle.h"
};

const uint8_t vector_heart[] = {
   #include "art/heart.h"
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
extern symbol_t* sym_sparkle1;
extern symbol_t* sym_sparkle2;
extern symbol_t* sym_sparkle3;

#define HEAD_X (CENTER_X)
#define HEAD_Y (CENTER_Y-50)

typedef enum {
   ANGRY    = 0,
   HAPPY    = 1,
   ECSTATIC = 2
} expression_t;

static void drawExpression(vector_t* vec, expression_t ix) {
   static expression_t last_expression = 0;
   if (ix != last_expression) {
      sym_expression->visible = false;
      waitVectorRefresh();
      switch (ix) {
      case HAPPY:
         memcpy( vec, vector_expression1, sizeof(vector_expression1) );
         break;
      case ANGRY:
         memcpy( vec, vector_expression2, sizeof(vector_expression2) );
         break;
      case ECSTATIC:
         memcpy( vec, vector_expression3, sizeof(vector_expression3) );
         break;
      }
      last_expression = ix;
      sym_expression->visible = true;
   }
}

static void drawSparkle(uint16_t x, uint16_t y, uint16_t angle, vector_t* vec) {
   symbol_t* sym = 0;
   if (!sym_sparkle1->visible) sym=sym_sparkle1;
   else if (!sym_sparkle2->visible) sym=sym_sparkle2;
   else if (!sym_sparkle3->visible) sym=sym_sparkle3;

   if (sym) {
      initSymbol( sym, vec );
      enableSymbol( sym, x, y, SEGA_ANGLE(0), 0x80 );
      setTrajectory( SID(sym), 5, angle );
      setResizeSpeed( SID(sym), -3 );
      setRotationSpeed( SID(sym), 20 + (rand8() & 0x3F) );
   }
}

static void drawHand(uint16_t vec_angle, uint8_t ear, uint8_t sparkle_count, vector_t* vec) {
   int16_t dx, dy, finger_x;
   vectorToXY( 0x03FF - vec_angle, 240, &dx, &dy);

   dy = divide1_5( dy );

   sym_hand1->visible = (dx > 0);
   sym_hand2->visible = (dx < 0);

   sym_hand1->x = HEAD_X - 0 + dx;
   sym_hand1->y = HEAD_Y + 50 - dy;
   sym_hand2->x = HEAD_X + 0 + dx;
   sym_hand2->y = HEAD_Y + 70 - dy;

   sym_hand1->rotation = (SEGA_ANGLE(0) + (dy >> 0)) & 0x03FF;
   sym_hand2->rotation = (SEGA_ANGLE(0) - (dy >> 0)) & 0x03FF;

   if (dy <= 0) {
      finger_x = sym_hand2->rotation;
   } else {
      finger_x = -((0x3FF - sym_hand2->rotation) >> 2);
   }

   if ( ear == 1 ) {
      // left ear
      uint16_t x = (int16_t)sym_hand1->x - finger_x;
      uint16_t y = sym_hand1->y + 50;
      for ( uint8_t i = 0; i < sparkle_count; i++ )
         drawSparkle( x, y, SEGA_ANGLE(225)+rand8(), vec );
   }

   if ( ear == 2 ) {
      // right ear
      uint16_t x = (int16_t)sym_hand2->x + 10 + finger_x;
      uint16_t y = sym_hand2->y + 50;
      for ( uint8_t i = 0; i < sparkle_count; i++ )
         drawSparkle( x, y, SEGA_ANGLE(45)+rand8(), vec );
   }
}


static void debugValue( uint16_t value ) {
   static uint16_t last_value = 0;
   static vector_t* vec   = NULL;
   if ( !vec ) {
      const char s[] = "8888";
      vec = ALLOC( measureString(s) * sizeof(vector_t) );
   }
   if ( value != last_value ) {
      last_value = value;
      char s[5] = {0,};
      sym_string3->visible = false;
      dec4( s, value );
      drawString( sym_string3, vec, CENTER_X, MIN_Y+40, 0x60, 0, s );
   }
}

void ferengiGame( void ) {

   vector_t* vec_head = ALLOC( sizeof(vector_head) );
   memcpy( vec_head, vector_head, sizeof(vector_head) );
   drawSymbol( sym_head, vec_head, HEAD_X, HEAD_Y+100, SEGA_ANGLE(0), 0x80 );

   vector_t* vec_ear1 = ALLOC( sizeof(vector_ear1) );
   memcpy( vec_ear1, vector_ear1, sizeof(vector_ear1) );
   drawSymbol( sym_ear1, vec_ear1, HEAD_X+188, HEAD_Y+78, SEGA_ANGLE(0), 0x80 );

   vector_t* vec_ear2 = ALLOC( sizeof(vector_ear2) );
   memcpy( vec_ear2, vector_ear2, sizeof(vector_ear2) );
   drawSymbol( sym_ear2, vec_ear2, HEAD_X-174, HEAD_Y+111, SEGA_ANGLE(0), 0x80 );

   vector_t* vec_shirt = ALLOC( sizeof(vector_shirt) );
   memcpy( vec_shirt, vector_shirt, sizeof(vector_shirt) );
   drawSymbol( sym_shirt, vec_shirt, HEAD_X+10, HEAD_Y-160, SEGA_ANGLE(0), 0x80 );

   vector_t* vec_expression = ALLOC( MAX(MAX(sizeof(vector_expression1),sizeof(vector_expression2)),sizeof(vector_expression3)) );
   memcpy( vec_expression, vector_expression2, sizeof(vector_expression2) );
   drawSymbol( sym_expression, vec_expression, HEAD_X+20, HEAD_Y+70, SEGA_ANGLE(0), 0x80 );

   const char s1[] = "rub lobes for oomox";
   vec_string1 = ALLOC( measureString(s1) * sizeof(vector_t) );
   drawString( sym_string1, vec_string1, GAME_TEXT_X, GAME_TEXT_Y, GAME_TEXT_SIZE, SEGA_COLOR_YELLOW, s1 );
   delayOrButton(4000);
   sym_string1->visible = false;
   FREE( vec_string1 );

   vector_t* vec_sparkle = ALLOC( sizeof(vector_sparkle) );
   memcpy( vec_sparkle, vector_sparkle, sizeof(vector_sparkle) );
   initSymbol( sym_sparkle1, vec_sparkle );
   initSymbol( sym_sparkle2, vec_sparkle );
   initSymbol( sym_sparkle3, vec_sparkle );

   vector_t* vec_heart = ALLOC( sizeof(vector_heart) );
   memcpy( vec_heart, vector_heart, sizeof(vector_heart) );

   vector_t* vec_hand1 = ALLOC( sizeof(vector_hand1) );
   memcpy( vec_hand1, vector_hand1, sizeof(vector_hand1) );
   initSymbol( sym_hand1, vec_hand1 );
   vector_t* vec_hand2 = ALLOC( sizeof(vector_hand2) );
   memcpy( vec_hand2, vector_hand2, sizeof(vector_hand2) );
   initSymbol( sym_hand2, vec_hand2 );

   uint16_t last_spawn_angle = 0;
   uint16_t last_angle = 0;

   spinner_vector_angle( SPIN_RESET );
   drawCountdown(30,false);

   uint16_t ear_tick        = system_tick - ((uint16_t)60 * 60 >> 4);  // first pick after SECONDS(1)
   uint16_t face_tick       = 0;
   uint16_t decay_tick      = 0;
   uint16_t friction        = 0;
   expression_t expr_tier   = ANGRY;
   uint8_t active_ear       = 0;  // 0=none, 1=left, 2=right

   bool oomox               = false;

#define FRICTION_MED      10   // angry -> happy
#define FRICTION_MED_LOW   7   // happy -> angry
#define FRICTION_HIGH     35   // happy -> ecstatic
#define FRICTION_HIGH_MED 30   // ecstatic -> happy
#define FRICTION_MAX      40   // cap to keep decay time reasonable
#define SPINNER_DELTA      4

   drawExpression( vec_expression, ANGRY );

   uint8_t cdown;
   uint8_t last_cdown = 0;
   uint16_t ear_interval = 0;
   uint16_t last_score = 0xffff;

   while ( (cdown = drawCountdown(0,false)) ) {

      waitAnimate(0);

      if ( score != last_score ) { drawScore( score, false ); last_score = score; }
      uint16_t angle = spinner_vector_angle( SPIN_FAST );

      if ( cdown != last_cdown ) {
         ear_interval = SECONDS(1) + ((uint16_t)cdown * cdown >> 4);
         last_cdown = cdown;
      }
      if ( system_tick - ear_tick > ear_interval ) {
         ear_tick = system_tick;
         friction = 0;

         active_ear = (rand8() & 0x01) ? 2 : 1;
         colorize( vec_ear1, sizeof(vector_ear1)/sizeof(vector_t), active_ear == 2 ? SEGA_COLOR_MAGENTA : SEGA_COLOR_ORANGE );
         colorize( vec_ear2, sizeof(vector_ear2)/sizeof(vector_t), active_ear == 1 ? SEGA_COLOR_MAGENTA : SEGA_COLOR_ORANGE );
      }

      // friction decay and face update always run, even when spinner is stationary
      if ( friction > 0 && system_tick - decay_tick >= SECONDS(0.1) ) {
         decay_tick = system_tick;
         friction--;
      }

      // Determine tier from friction with hysteresis
      expression_t new_tier = expr_tier;
      switch (expr_tier) {
         case ANGRY:    if (friction >= FRICTION_MED)      new_tier = HAPPY;     break;
         case HAPPY:    if (friction >= FRICTION_HIGH)     new_tier = ECSTATIC;
                   else if (friction <  FRICTION_MED_LOW)  new_tier = ANGRY;     break;
         case ECSTATIC: if (friction <  FRICTION_HIGH_MED) new_tier = HAPPY;  oomox=true;   break;
      }

      // Rate-limit face changes to every 1 second
      if ( new_tier != expr_tier && system_tick - face_tick > SECONDS(1) ) {
         if ( new_tier > expr_tier ) { if ( rand8() & 0x01 ) say(FERENGI_AH); else say(FERENGI_YES); }
         expr_tier = new_tier;
         face_tick = system_tick;
         drawExpression( vec_expression, expr_tier );
      }

      static uint16_t l_angle = 0;
      int16_t delta = (int16_t)(angle - l_angle);
      if (delta < 0) delta = -delta;
      bool moving = (delta >= SPINNER_DELTA);
      l_angle = angle;

      bool in_right_ear = (angle > SEGA_ANGLE(42) && angle < SEGA_ANGLE(110));
      bool in_left_ear  = (angle > SEGA_ANGLE(260) && angle < SEGA_ANGLE(323));

      bool in_active_ear = (active_ear == 1 && in_right_ear) || (active_ear == 2 && in_left_ear);
      uint8_t ear = (moving && in_active_ear) ? active_ear : 0;

      static uint8_t button_last = 0xff;
      volatile uint8_t button = PORT_374;
      if ( ear ) {
         if ( friction < FRICTION_MAX ) friction++;
         uint8_t button_edge = (button_last ^ button) & ~button; // falling edge only
         if (button_edge & BUTTON_FIRE) {
            friction+=10; // extra friction on that ear!
         }
      }
      button_last = button;

      if ( moving ) {
         static uint8_t sparkle_counter = 0;
         if (ear) sparkle_counter++;
         uint8_t sparkle_count = (expr_tier == ECSTATIC && !(sparkle_counter & 1)) ? 1 :
                                 (expr_tier == HAPPY    && !(sparkle_counter & 7)) ? 1 : 0;
         drawHand( angle, ear, sparkle_count, (expr_tier == ECSTATIC) ? vec_heart : vec_sparkle );
         if ( ear ) score += sparkle_count;
      }

   }


   sym_sparkle1->visible = false;
   sym_sparkle2->visible = false;
   sym_sparkle3->visible = false;
   sym_hand1->visible = false;
   sym_hand2->visible = false;
   colorize( vec_ear1, sizeof(vector_ear1)/sizeof(vector_t), SEGA_COLOR_ORANGE );
   colorize( vec_ear2, sizeof(vector_ear2)/sizeof(vector_t), SEGA_COLOR_ORANGE );

   drawExpression( vec_expression, HAPPY );
   delay(1000);
   if (oomox) {
      say(FERENGI_OOMOX);
      delay(6000);
   }

   sym_head->visible = false;
   sym_ear1->visible = false;
   sym_ear2->visible = false;
   sym_shirt->visible = false;
   sym_expression->visible = false;
   FREE( vec_sparkle );
   FREE( vec_heart );
   FREE( vec_head );
   FREE( vec_ear1 );
   FREE( vec_ear2 );
   FREE( vec_shirt );
   FREE( vec_expression );
   FREE( vec_hand1 );
   FREE( vec_hand2 );
}
