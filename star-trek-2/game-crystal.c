#include "sega.h"



const uint8_t vector_ring[] = {
    #include "art/ring.h"
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

extern symbol_t* sym_hand1;
extern symbol_t* sym_latinum1;
extern symbol_t* sym_latinum2;
extern symbol_t* sym_latinum3;
extern symbol_t* sym_shirt1;
extern symbol_t* sym_shirt2;
extern symbol_t* sym_shirt3;

// Target slot positions inside the jar housing
static const int16_t CRYSTAL_SLOT_X[3] = { CENTER_X-100, CENTER_X, CENTER_X+100 };
static const int16_t CRYSTAL_SLOT_Y[3] = { CENTER_Y+10,  CENTER_Y-30, CENTER_Y+10 };
static const uint16_t CRYSTAL_SLOT_ROT[3] = { 967, 0, 56 };

// Difficulty ramps per crystal
static const uint8_t DRIFT_STRENGTH[3]  = { 1, 2, 3 };

// Auto-lock threshold
#define LOCK_THRESHOLD_XY  15

// Gauge: map distance to inverted scale (0xFF=perfect, 0x10=bad)
// max_shift = log2 of max distance
static uint8_t gaugeScale( int16_t dist, uint8_t max_shift ) {
   if ( dist < 0 ) dist = -dist;
   if ( dist >= ((int16_t)1 << max_shift) ) return 0x10;
   uint16_t scaled;
   if ( max_shift <= 8 ) {
      scaled = (uint16_t)dist << (8 - max_shift);
   } else {
      scaled = (uint16_t)dist >> (max_shift - 8);
   }
   if ( scaled > 0xEF ) return 0x10;
   uint8_t result = 0xFF - (uint8_t)scaled;
   if ( result < 0x10 ) result = 0x10;
   return result;
}

// 16-direction unit vectors (10-bit angle: 0=up, 256=right, 512=down, 768=left)
static const int8_t RING_DX[16] = {  0, 1, 2, 2, 2, 2, 2, 1, 0,-1,-2,-2,-2,-2,-2,-1 };
static const int8_t RING_DY[16] = { -2,-2,-2,-1, 0, 1, 2, 2, 2, 2, 2, 1, 0,-1,-2,-2 };

// Push crystal in the direction of ring_angle
static void ringPush( uint16_t angle, int16_t* cx, int16_t* cy ) {
   uint8_t d = (uint8_t)((angle + 32) >> 6) & 0x0F;
   *cx -= (int16_t)RING_DX[d] * 3;
   *cy += (int16_t)RING_DY[d] * 3;
}

void crystalGame( void ) {

   const char s1[] = "stabilize dilithium crystal";
   vec_string1 = ALLOC( measureString(s1) * sizeof(vector_t) );
   drawString( sym_string1, vec_string1, CENTER_X-460, MIN_Y+40, 0x40, SEGA_COLOR_YELLOW, s1 );

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

   say( POWER_FAILED );
   delay( 800 );

   // === Pop crystals out with setTrajectory — they fly outward and stop ===
   SOUND_COMMAND = NOMAD_MOTION;
   for ( uint8_t i = 0; i < 3; i++ ) {
      setTrajectory( SID(crystal_sym[i]), 8, rand16() & 0x03ff  );
      setRotationSpeed( SID(crystal_sym[i]), 6 + (rand8()&7) );
   }
   waitAnimate( SECONDS(0.5) );
   for ( uint8_t i = 0; i < 3; i++ ) {
      setStop( SID(crystal_sym[i]) );
   }
   SOUND_COMMAND = NOMAD_MOTION_END;

   spinner_vector_angle( true );
   drawCountdown( 20 );

   uint8_t locked_count = 0;

   for ( uint8_t ci = 0; ci < 3; ci++ ) {
      if ( !drawCountdown(0) ) break;

      // --- Show ghost crystal at target slot ---
      memcpy( vec_ghost, vec_c[ci], crystal_bytes[ci] );
      colorize( vec_ghost, crystal_nvecs[ci], SEGA_COLOR_GRAY );
      initSymbol( sym_latinum1, vec_ghost );
      drawSymbol( sym_latinum1, vec_ghost,
                  CRYSTAL_SLOT_X[ci], CRYSTAL_SLOT_Y[ci],
                  CRYSTAL_SLOT_ROT[ci], 0x60 );

      // --- Ring: always visible, rotates with spinner ---
      drawSymbol( sym_shirt1, vec_ring_art, CENTER_X, CENTER_Y, 0, 0xE0 );

      // --- Laser: hidden until fire is pressed ---
      drawSymbol( sym_shirt2, vec_laser_art, CENTER_X, CENTER_Y, 0, 0xE0 );
      sym_shirt2->visible = false;

      // --- Active crystal color ---
      colorize( vec_c[ci], crystal_nvecs[ci], SEGA_COLOR_MAGENTA );

      int16_t cx = crystal_sym[ci]->x;
      int16_t cy = crystal_sym[ci]->y;

      bool locked = false;
      uint8_t fire_debounce = 0;
      uint8_t fire_count = 0;
      uint8_t laser_flash = 0;
      uint8_t prev_buttons = 0;
      uint16_t drift_tick = system_tick;

      while ( !locked && drawCountdown(0) ) {

         waitAnimate(0);
         drawScore( score, false );

         // --- Read controls ---
         uint8_t buttons = PORT_374;
         uint16_t ring_angle = spinner_vector_angle( false );

         // Spinner → ring and laser rotation
         sym_shirt1->rotation = ring_angle;
         sym_shirt2->rotation = ring_angle;

         // --- Laser flash: show for a few frames after fire ---
         if ( laser_flash ) {
            laser_flash--;
            sym_shirt2->visible = (laser_flash > 0);
         }

         // --- Fire button: push crystal and flash laser ---

         if ( !(prev_buttons & BUTTON_FIRE) ) fire_count = 0;
         if ( fire_debounce ) fire_debounce--;
         if ( (buttons & BUTTON_FIRE) && !fire_debounce && fire_count < 6 ) {
            fire_debounce = 4;
            fire_count++;
            ringPush( ring_angle, &cx, &cy );
            if ( fire_count == 1 ) {
               sym_shirt2->visible = true;
               laser_flash = 6;
               SOUND_COMMAND = PHASER;
            }
         }
         prev_buttons = buttons;

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

         // Gently auto-rotate crystal toward its slot rotation
         int16_t drot = (int16_t)(CRYSTAL_SLOT_ROT[ci] - crystal_sym[ci]->rotation) & 0x3FF;
         if ( drot > 512 ) drot -= 1024;
         if      ( drot >  2 ) crystal_sym[ci]->rotation = (crystal_sym[ci]->rotation + 2) & 0x3FF;
         else if ( drot < -2 ) crystal_sym[ci]->rotation = (crystal_sym[ci]->rotation - 2) & 0x3FF;
         else                  crystal_sym[ci]->rotation = CRYSTAL_SLOT_ROT[ci];

         crystal_sym[ci]->x = cx;
         crystal_sym[ci]->y = cy;

         // --- Calculate alignment ---
         int16_t dx = ABS( cx - CRYSTAL_SLOT_X[ci] );
         int16_t dy = ABS( cy - CRYSTAL_SLOT_Y[ci] );
         uint16_t dist = dx + dy;

         // --- Crystal color based on proximity ---
         uint8_t g_pos = gaugeScale( dist, 9 );
         if      ( g_pos > 0xC0 ) colorize( vec_c[ci], crystal_nvecs[ci], SEGA_COLOR_BRWHITE );
         else if ( g_pos > 0x80 ) colorize( vec_c[ci], crystal_nvecs[ci], SEGA_COLOR_CYAN );
         else if ( g_pos > 0x40 ) colorize( vec_c[ci], crystal_nvecs[ci], SEGA_COLOR_GREEN );
         else                     colorize( vec_c[ci], crystal_nvecs[ci], SEGA_COLOR_MAGENTA );

         // --- Jar color tracks proximity ---
         if      ( dx < LOCK_THRESHOLD_XY && dy < LOCK_THRESHOLD_XY ) colorize( vec_jar, jar_nvecs, SEGA_COLOR_GREEN );
         else if ( g_pos > 0x80 ) colorize( vec_jar, jar_nvecs, SEGA_COLOR_YELLOW );
         else if ( g_pos > 0x40 ) colorize( vec_jar, jar_nvecs, SEGA_COLOR_CYAN );
         else                     colorize( vec_jar, jar_nvecs, SEGA_COLOR_BLUE );

         // --- Auto-lock when crystal is close enough ---
         if ( dx < LOCK_THRESHOLD_XY && dy < LOCK_THRESHOLD_XY ) {
            locked = true;
            locked_count++;
            score += 2 + (ci << 1);
            SOUND_COMMAND = DOCK;

            sym_latinum1->visible = false;

            // Snap crystal to slot
            crystal_sym[ci]->x        = CRYSTAL_SLOT_X[ci];
            crystal_sym[ci]->y        = CRYSTAL_SLOT_Y[ci];
            crystal_sym[ci]->rotation = CRYSTAL_SLOT_ROT[ci];
            colorize( vec_c[ci], crystal_nvecs[ci], SEGA_COLOR_BRWHITE );

            // Flash celebration
            for ( uint8_t f = 0; f < 6; f++ ) {
               colorize( vec_jar, jar_nvecs, (f & 1) ? SEGA_COLOR_GREEN : SEGA_COLOR_CYAN );
               waitAnimate( 4 );
            }
         }

      } // end while for this crystal

      // --- Per-crystal cleanup ---
      sym_shirt1->visible   = false;
      sym_shirt2->visible   = false;
      sym_latinum1->visible = false;

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
         drawString( sym_string1, vec_string1, CENTER_X-280, MIN_Y+40, 0x90, SEGA_COLOR_GREEN, s );
      }
      waitAnimate( SECONDS(3) );

      sym_string1->visible = false;
      FREE( vec_string1 );

   } else {
      SOUND_COMMAND = ENTERPRISE_EXPLOSION;
      {
         const char s[] = "core breach";
         vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );
         drawString( sym_string1, vec_string1, CENTER_X-240, MIN_Y+40, 0x90, SEGA_COLOR_RED, s );
      }
      setRotationSpeed( SID(sym_jar), 60 );
      setResizeSpeed( SID(sym_jar), -8 );
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
   sym_shirt1->visible   = false;
   sym_shirt2->visible   = false;
   sym_latinum1->visible = false;
   FREE( vec_jar );
   FREE( vec_c[0] );
   FREE( vec_c[1] );
   FREE( vec_c[2] );
   FREE( vec_ghost );
   FREE( vec_ring_art );
   FREE( vec_laser_art );
}
