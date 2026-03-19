#include "sega.h"



const uint8_t vector_ring[] = {
    #include "art/ring.h"
};

const uint8_t vector_circle[] = {
    #include "art/circle.h"
};

const uint8_t vector_laser[] = {
    #include "art/laser.h"
};

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



extern symbol_t* sym_jar;
extern symbol_t* sym_crystal1;
extern symbol_t* sym_crystal2;
extern symbol_t* sym_crystal3;

#define sym_ghost    sym_latinum1
#define sym_circle1  sym_latinum2
#define sym_circle2  sym_latinum3
#define sym_ring     sym_shirt1
#define sym_laser    sym_shirt2

extern symbol_t* sym_latinum1;
extern symbol_t* sym_latinum2;
extern symbol_t* sym_latinum3;
extern symbol_t* sym_shirt1;
extern symbol_t* sym_shirt2;

// Target slot positions inside the jar housing
static const int16_t CRYSTAL_SLOT_X[3] = { CENTER_X-100, CENTER_X, CENTER_X+100 };
static const int16_t CRYSTAL_SLOT_Y[3] = { CENTER_Y+10,  CENTER_Y-30, CENTER_Y+10 };
static const uint16_t CRYSTAL_SLOT_ROT[3] = { 967, 0, 56 };

// Difficulty ramps per crystal
static const uint8_t DRIFT_STRENGTH[3]  = { 1, 2, 3 };

// Auto-lock threshold
#define LOCK_THRESHOLD_XY  15

// 16-direction unit vectors (10-bit angle: 0=up, 256=right, 512=down, 768=left)
static const int8_t RING_DX[16] = {  0, 4, 8, 8, 8, 8, 8, 4, 0,-4,-8,-8,-8,-8,-8,-4 };
static const int8_t RING_DY[16] = { -8,-8,-8,-4, 0, 4, 8, 8, 8, 8, 8, 4, 0,-4,-8,-8 };

// Push crystal in the direction of ring_angle
static void ringPush( uint16_t angle, int16_t* cx, int16_t* cy ) {
   uint8_t d = (uint8_t)((angle + 32) >> 6) & 0x0F;
   *cx -= (int16_t)RING_DX[d];
   *cy += (int16_t)RING_DY[d];
}

void crystalGame( void ) {

   // === Allocate and copy vector art ===
   vector_t* vec_jar = ALLOC( sizeof(vector_jar) );
   memcpy( vec_jar, vector_jar, sizeof(vector_jar) );

   vector_t* vec_c[3];
   vec_c[0] = ALLOC( sizeof(vector_crystal1) );
   memcpy( vec_c[0], vector_crystal1, sizeof(vector_crystal1) );
   vec_c[1] = ALLOC( sizeof(vector_crystal2) );
   memcpy( vec_c[1], vector_crystal2, sizeof(vector_crystal2) );
   vec_c[2] = ALLOC( sizeof(vector_crystal3) );
   memcpy( vec_c[2], vector_crystal3, sizeof(vector_crystal3) );

   const uint16_t crystal_nvecs[3] = {
      sizeof(vector_crystal1) / sizeof(vector_t),
      sizeof(vector_crystal2) / sizeof(vector_t),
      sizeof(vector_crystal3) / sizeof(vector_t)
   };
   const uint16_t jar_nvecs = sizeof(vector_jar) / sizeof(vector_t);
   symbol_t* crystal_sym[3] = { sym_crystal1, sym_crystal2, sym_crystal3 };

   // Crystal byte sizes for ghost copy (nvecs << 2 = nvecs * sizeof(vector_t))
   const uint16_t crystal_bytes[3] = {
      crystal_nvecs[0] << 2,
      crystal_nvecs[1] << 2,
      crystal_nvecs[2] << 2
   };

   // === Allocate ghost crystal ===
   uint16_t ghost_sz = MAX( MAX( sizeof(vector_crystal1), sizeof(vector_crystal2) ), sizeof(vector_crystal3) );
   vector_t* vec_ghost = ALLOC( ghost_sz );

   // === Allocate ring and laser art ===
   vector_t* vec_ring_art = ALLOC( sizeof(vector_ring) );
   memcpy( vec_ring_art, vector_ring, sizeof(vector_ring) );

   vector_t* vec_circle_art = ALLOC( sizeof(vector_circle) );
   memcpy( vec_circle_art, vector_circle, sizeof(vector_circle) );

   vector_t* vec_laser_art = ALLOC( sizeof(vector_laser) );
   memcpy( vec_laser_art, vector_laser, sizeof(vector_laser) );

   // === Draw the matrix housing ===
   drawSymbol( sym_jar, vec_jar, CENTER_X, CENTER_Y, SEGA_ANGLE(0), 0xE0 );
   colorize( vec_jar, jar_nvecs, SEGA_COLOR_CYAN );

   // Show crystals seated in home slots
   for ( uint8_t i = 0; i < 3; i++ ) {
      drawSymbol( crystal_sym[i], vec_c[i], CRYSTAL_SLOT_X[i], CRYSTAL_SLOT_Y[i],
                  CRYSTAL_SLOT_ROT[i], 0x60 );
   }

   const char s1[] = "laser crystals to center";
   vec_string1 = ALLOC( measureString(s1) * sizeof(vector_t) );
   drawString( sym_string1, vec_string1, GAME_TEXT_X, GAME_TEXT_Y, GAME_TEXT_SIZE, SEGA_COLOR_YELLOW, s1 );
   delay(3000);
   sym_string1->visible = false;
   FREE( vec_string1 );

   say( POWER_FAILED );
   delay( 800 );

   // === Pop crystals out with setTrajectory — they fly outward and stop ===
   SOUND_COMMAND = NOMAD_MOTION;
   for ( uint8_t i = 0; i < 3; i++ ) {
      setTrajectory( SID(crystal_sym[i]), 10, rand16() & 0x03ff  );
      setRotationSpeed( SID(crystal_sym[i]), 33 );
   }
   waitAnimate( SECONDS(0.5) );
   for ( uint8_t i = 0; i < 3; i++ ) {
      setStop( SID(crystal_sym[i]) );
   }
   SOUND_COMMAND = NOMAD_MOTION_END;

   spinner_vector_angle( true );
   drawCountdown( 25, false );

   uint8_t locked_count = 0;

   for ( uint8_t ci = 0; ci < 3; ci++ ) {
      if ( !drawCountdown(0,true) ) break;

      // --- Show ghost crystal at target slot ---
      memcpy( vec_ghost, vec_c[ci], crystal_bytes[ci] );
      colorize( vec_ghost, crystal_nvecs[ci], SEGA_COLOR_BLUE );
      initSymbol( sym_ghost, vec_ghost );
      drawSymbol( sym_ghost, vec_ghost,
                  CRYSTAL_SLOT_X[ci], CRYSTAL_SLOT_Y[ci],
                  CRYSTAL_SLOT_ROT[ci], 0x60 );

      // --- Ring: always visible, rotates with spinner ---
      drawSymbol( sym_ring, vec_ring_art, CENTER_X, CENTER_Y, 0, 0xC0 );
      drawSymbol( sym_circle1, vec_circle_art, CENTER_X, CENTER_Y, 0, 0xB8 );
      drawSymbol( sym_circle2, vec_circle_art, CENTER_X, CENTER_Y, 0, 0xC0 );

      // --- Laser: hidden until fire is pressed ---
      drawSymbol( sym_laser, vec_laser_art, CENTER_X, CENTER_Y, 0, 0xB8 );
      sym_laser->visible = false;

      int16_t cx = crystal_sym[ci]->x;
      int16_t cy = crystal_sym[ci]->y;

      bool locked = false;
      uint16_t fire_tick = 0;
      uint8_t prev_buttons = 0;
      uint16_t drift_tick = system_tick;
      uint8_t last_color = 0;

      while ( !locked && drawCountdown(0,true) ) {

         waitAnimate(0);
         drawScore( score, false );

         // --- Read controls ---
         uint8_t buttons = PORT_374;
         uint16_t ring_angle = spinner_vector_angle( false );

         // Spinner → ring and laser rotation
         sym_ring->rotation = ring_angle;
         sym_laser->rotation = ring_angle;

         bool fire_active = fire_tick && (system_tick - fire_tick < SECONDS(0.1));
         sym_laser->visible = fire_active;

         // --- Magnetic drift ---
         if ( system_tick - drift_tick >= 3 ) {
            drift_tick = system_tick;
            uint8_t r = rand8();
            int8_t ds = DRIFT_STRENGTH[ci];
            cx += (r & 0x01) ? ds : -ds;
            cy += (r & 0x02) ? ds : -ds;
         }

         // Clamp to viewport
         if ( cx < MIN_X + 40 ) cx = MIN_X + 40;
         if ( cx > MAX_X - 40 ) cx = MAX_X - 40;
         if ( cy < MIN_Y + 40 ) cy = MIN_Y + 40;
         if ( cy > MAX_Y - 40 ) cy = MAX_Y - 40;

         crystal_sym[ci]->x = cx;
         crystal_sym[ci]->y = cy;

         // --- Fire button: push crystal and flash laser ---

         if ( (buttons & BUTTON_FIRE) && !(prev_buttons & BUTTON_FIRE) ) {
            fire_tick = system_tick;
            ringPush( ring_angle, &cx, &cy );
            SOUND_COMMAND = PHASER;
         }
         prev_buttons = buttons;

         // After laser fire: auto-rotate and update color throughout debounce window
         if ( fire_active ) {
            int16_t drot = (int16_t)(CRYSTAL_SLOT_ROT[ci] - crystal_sym[ci]->rotation) & 0x3FF;
            if ( drot > 512 ) drot -= 1024;
            if      ( drot >  2 ) crystal_sym[ci]->rotation = (crystal_sym[ci]->rotation + 2) & 0x3FF;
            else if ( drot < -2 ) crystal_sym[ci]->rotation = (crystal_sym[ci]->rotation - 2) & 0x3FF;
            else                  crystal_sym[ci]->rotation = CRYSTAL_SLOT_ROT[ci];

            // --- Crystal color based on proximity (only colorize on change) ---
            int16_t dx = ABS( cx - CRYSTAL_SLOT_X[ci] );
            int16_t dy = ABS( cy - CRYSTAL_SLOT_Y[ci] );
            uint16_t dist = dx + dy;
            uint8_t new_color = ( dist < 256 ) ? SEGA_COLOR_MAGENTA :
                                ( dist < 384 ) ? SEGA_COLOR_YELLOW  : SEGA_COLOR_CYAN;
            if ( new_color != last_color ) {
               colorize( vec_c[ci], crystal_nvecs[ci], new_color );
               last_color = new_color;
            }
         }

         // --- Auto-lock when crystal is close enough ---
         {  int16_t dx = ABS( cx - CRYSTAL_SLOT_X[ci] );
            int16_t dy = ABS( cy - CRYSTAL_SLOT_Y[ci] );
            if ( dx < LOCK_THRESHOLD_XY && dy < LOCK_THRESHOLD_XY ) {
               locked = true;
               locked_count++;
               score += 2 + (ci << 1);
               SOUND_COMMAND = DOCK;

               sym_ghost->visible = false;
               sym_laser->visible = false;

               // Snap crystal to slot
               crystal_sym[ci]->x        = CRYSTAL_SLOT_X[ci];
               crystal_sym[ci]->y        = CRYSTAL_SLOT_Y[ci];
               crystal_sym[ci]->rotation = CRYSTAL_SLOT_ROT[ci];

               // Flash celebration
               for (uint8_t color = 0; color < 0x3F; color++) {
                  colorize( vec_c[ci], crystal_nvecs[ci], color );
                  static uint16_t lt = 0;
                  while ( system_tick == lt ) { __asm__( "nop" ); }
                  lt = system_tick;
               }
            }
         } // end auto-lock block

      } // end while for this crystal

      // --- Per-crystal cleanup ---
      sym_ring->visible   = false;
      sym_circle1->visible = false;
      sym_circle2->visible = false;
      sym_laser->visible   = false;
      sym_ghost->visible = false;

      if ( !locked ) {
         crystal_sym[ci]->visible = false;
      }

   } // end for each crystal

   sym_string1->visible = false;
   FREE( vec_string1 );

   // --- End sequence ---
   if ( locked_count == 3 ) {
      say( POWER_RESTORED );
      score += 5;

      {
         const char s[] = "matrix stable";
         vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );
         drawString( sym_string1, vec_string1, CENTER_X-200, GAME_TEXT_Y, GAME_TEXT_SIZE, SEGA_COLOR_GREEN, s );
      }
      waitAnimate( SECONDS(3) );

      sym_string1->visible = false;
      FREE( vec_string1 );

   } else {
      SOUND_COMMAND = ENTERPRISE_EXPLOSION;
      {
         const char s[] = "core breach";
         vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );
         drawString( sym_string1, vec_string1, CENTER_X-200, GAME_TEXT_Y, GAME_TEXT_SIZE, SEGA_COLOR_RED, s );
      }
      setRotationSpeed( SID(sym_jar), 60 );
      setResizeSpeed( SID(sym_jar), 16 );
      waitAnimate( SECONDS(2) );
      setStop( SID(sym_jar) );
      sym_string1->visible = false;
      FREE( vec_string1 );
   }

   // --- Final cleanup ---
   sym_jar->visible      = false;
   sym_crystal1->visible = false;
   sym_crystal2->visible = false;
   sym_crystal3->visible = false;
   sym_ring->visible   = false;
   sym_laser->visible   = false;
   sym_ghost->visible = false;
   FREE( vec_jar );
   FREE( vec_c[0] );
   FREE( vec_c[1] );
   FREE( vec_c[2] );
   FREE( vec_ghost );
   FREE( vec_ring_art );
   FREE( vec_laser_art );
}
