#include "sega.h"


const uint8_t vector_planet[] = {
    #include "art/planet.h"
};

const uint8_t vector_ship[] = {
    #include "art/ship.h"
};

const uint8_t vector_target[] = {
    #include "art/target.h"
};

extern symbol_t* sym_planet[NUM_PLANETS];
extern symbol_t* sym_planet_label[NUM_PLANETS];
extern symbol_t* sym_fleet[MAX_FLEETS];
extern symbol_t* sym_fleet_label[MAX_FLEETS];
extern symbol_t* sym_cursor;

#define PLANET_NVECS  (sizeof(vector_planet) / sizeof(vector_t))

#define PLANET_MARGIN   110   // keep planet centers away from the screen edges
#define MIN_SEPARATION  150   // minimum x-or-y gap between any two planet centers
#define MAX_PLACEMENT_ATTEMPTS 200  // give up and accept a cramped spot rather than loop forever
#define LABEL_SCALE   0x50
#define LABEL_DX      (-20)
#define LABEL_DY      (-20)
#define FLEET_SPEED   2
#define FLEET_SCALE   0x50
#define DETENT_SIZE   40   // spinner units per planet step
#define EXCHANGE_INTERVAL SECONDS(0.25)  // how often a fleet and planet trade hits
#define HIT_MIN  1
#define HIT_MASK 0x03      // per-hit damage is HIT_MIN + (0..HIT_MASK), so 1-4
// at FLEET_SPEED's low magnitude, only a handful of integer (vx,vy) pairs are
// representable, so any single heading can be several degrees off the true
// one. re-aiming partway through the flight bounds the drift instead of
// letting one rounding choice commit the whole trip.
#define REAIM_INTERVAL SECONDS(0.5)
// growth is staggered one planet at a time across each half-second, rather
// than updating all of them in the same tick (both cheaper per-frame and
// reads more organically). compile-time division of two compile-time
// constants, so this is resolved by the compiler, not a runtime divide.
#define GROWTH_STEP_INTERVAL (SECONDS(0.5) / NUM_PLANETS)
#define AI_THINK_INTERVAL SECONDS(4)
#define AI_MIN_ATTACK_SHIPS 5

typedef enum {
   OWNER_NEUTRAL = 0,
   OWNER_PLAYER  = 1,
   OWNER_AI      = 2,
} owner_t;

// cycle small/medium/large for "various sizes" (assigned to shuffled planet slots)
static const uint8_t PLANET_TIER[NUM_PLANETS] = { 0,1,2, 0,1,2, 0,1,2, 0 };
#define TIER_SCALE_0 0x50
#define TIER_SCALE_1 0x80
#define TIER_SCALE_2 0xB0
static const uint8_t TIER_SCALE[3]  = { TIER_SCALE_0, TIER_SCALE_1, TIER_SCALE_2 };

#define PLANET_BASE_RADIUS 20
#define PARK_THRESHOLD 40  // "close enough" to its parking point to call it arrived

// every planet tier's scale is a fixed constant, so these radii are resolved
// entirely at compile time -- no runtime multiply
static const uint8_t PLANET_RADIUS[3] = {
   (PLANET_BASE_RADIUS * TIER_SCALE_0) >> 7,
   (PLANET_BASE_RADIUS * TIER_SCALE_1) >> 7,
   (PLANET_BASE_RADIUS * TIER_SCALE_2) >> 7,
};

typedef struct {
   uint16_t x, y;
   uint8_t  scale;
   uint8_t  growth;   // value gained per second tick
   uint8_t  value;    // ships/score currently held
   uint8_t  owner;    // owner_t: who currently holds this planet
   uint16_t battle_tick; // shared exchange timer for every fleet attacking this planet
   vector_t* vec_label;
} planet_t;

static planet_t planets[NUM_PLANETS];

// planets are placed at random, so their array indices have no relation to
// screen position. cursor_order lists planet indices sorted by angle around
// the screen center, and the spinner steps through that instead of raw index
// order -- a rotary control naturally maps to circular motion, so spinning
// one way always sweeps the cursor around like a clock, rather than jumping
// between arbitrary rows the way left-to-right/top-to-bottom ordering would.
static uint8_t cursor_order[NUM_PLANETS];

// every planet is the same shape; only 3 colored copies ever exist, and each
// planet symbol just points at whichever one matches its current owner
static vector_t* vec_planet_neutral;
static vector_t* vec_planet_player;
static vector_t* vec_planet_ai;

static inline vector_t* planetVec( uint8_t ix ) {
   if ( planets[ix].owner == OWNER_PLAYER ) return vec_planet_player;
   if ( planets[ix].owner == OWNER_AI )     return vec_planet_ai;
   return vec_planet_neutral;
}

static void updatePlanetLabel( uint8_t ix ) {
   char s[3] = {0,};
   dec2( s, planets[ix].value );
   drawString( sym_planet_label[ix], planets[ix].vec_label,
               planets[ix].x + LABEL_DX, planets[ix].y + LABEL_DY,
               LABEL_SCALE, SEGA_COLOR_WHITE, s );
}

static void buildCursorOrder( void ) {
   uint16_t angle[NUM_PLANETS];
   for ( uint8_t i = 0; i < NUM_PLANETS; i++ ) {
      cursor_order[i] = i;
      angle[i] = deltaToVector( (int16_t)planets[i].x - CENTER_X, (int16_t)planets[i].y - CENTER_Y );
   }

   // sort by angle ascending (small n: plain insertion sort is plenty fast)
   for ( uint8_t i = 1; i < NUM_PLANETS; i++ ) {
      uint8_t key = cursor_order[i];
      uint16_t key_angle = angle[key];
      int8_t j = (int8_t)i - 1;
      while ( j >= 0 && angle[cursor_order[j]] > key_angle ) {
         cursor_order[j+1] = cursor_order[j];
         j--;
      }
      cursor_order[j+1] = key;
   }
}

static uint8_t orderPositionOf( uint8_t ix ) {
   for ( uint8_t p = 0; p < NUM_PLANETS; p++ ) {
      if ( cursor_order[p] == ix ) return p;
   }
   return 0;
}

static uint8_t nextSelectable( uint8_t ix ) {
   uint8_t pos = orderPositionOf(ix);
   pos = (pos + 1 == NUM_PLANETS) ? 0 : pos + 1;
   return cursor_order[pos];
}

static uint8_t prevSelectable( uint8_t ix ) {
   uint8_t pos = orderPositionOf(ix);
   pos = (pos == 0) ? NUM_PLANETS - 1 : pos - 1;
   return cursor_order[pos];
}

// one fleet per planet that can be attacking/reinforcing at once (every owned
// planet launches its own fleet when fire is pressed, so multiple can be in
// flight -- or fighting the same target -- simultaneously; the AI's fleets
// share this same pool)
typedef struct {
   bool     active;
   bool     battling;
   uint8_t  owner;   // owner_t: which side sent it
   uint8_t  target;
   uint8_t  ships;
   uint16_t retarget_tick; // last time its trajectory was recomputed mid-flight
   uint16_t park_x, park_y; // this fleet's own spot on the target's boundary
} fleet_t;

static fleet_t fleets[MAX_FLEETS];

static void updateFleetLabel( uint8_t f, vector_t* vec_fleet_label[] ) {
   char fs[3] = {0,};
   dec2( fs, fleets[f].ships );
   drawString( sym_fleet_label[f], vec_fleet_label[f],
               sym_fleet[f]->x + LABEL_DX, sym_fleet[f]->y + LABEL_DY,
               LABEL_SCALE, SEGA_COLOR_WHITE, fs );
}

// launches every ship currently at planet[src] as a new fleet toward target,
// used by both the player's fire button and the AI's own decisions
static void launchFleet( uint8_t src, uint8_t target, uint8_t owner,
                         vector_t* ship_art, vector_t* vec_fleet_label[] ) {
   for ( uint8_t f = 0; f < MAX_FLEETS; f++ ) {
      if ( fleets[f].active ) continue;

      fleets[f].active   = true;
      fleets[f].battling = false;
      fleets[f].owner    = owner;
      fleets[f].target   = target;
      fleets[f].ships    = planets[src].value;
      fleets[f].retarget_tick = system_tick;
      planets[src].value = 0;
      updatePlanetLabel( src );

      int16_t dx = (int16_t)planets[target].x - (int16_t)planets[src].x;
      int16_t dy = (int16_t)planets[target].y - (int16_t)planets[src].y;
      uint16_t heading = deltaToVector( dx, dy );

      // stop at the target's edge (scaled to its size), not its exact
      // center -- along the same line it's approaching from
      uint16_t standoff = PLANET_RADIUS[ PLANET_TIER[target] ];
      int16_t pdx, pdy;
      vectorToXY( (heading + 512) & 0x3FF, standoff, &pdx, &pdy );
      fleets[f].park_x = planets[target].x + pdx;
      fleets[f].park_y = planets[target].y + pdy;

      drawSymbol( sym_fleet[f], ship_art, planets[src].x, planets[src].y, heading, FLEET_SCALE );
      updateFleetLabel( f, vec_fleet_label );

      setTrajectory( SID(sym_fleet[f]), FLEET_SPEED, heading );
      setTrajectory( SID(sym_fleet_label[f]), FLEET_SPEED, heading );

      SOUND_COMMAND = PHASER;
      return; // launched; this planet's ships are spoken for
   }
}

// picks a random planet (other than 'avoid', if given -- pass 0xFF for no
// constraint) and hands it to owner, recoloring its symbol to match
static uint8_t claimRandomPlanet( uint8_t owner, vector_t* vec, uint8_t avoid ) {
   uint8_t ix;
   do {
      ix = rand8() & 0x0F;
   } while ( ix >= NUM_PLANETS || ix == avoid );
   planets[ix].owner = owner;
   sym_planet[ix]->vector_addr = (uint16_t)vec;
   return ix;
}

void planetGame( void ) {

   // === one shape, three colored copies shared by all planets ===
   vec_planet_neutral = ALLOC( sizeof(vector_planet) );
   memcpy( vec_planet_neutral, vector_planet, sizeof(vector_planet) );
   colorize( vec_planet_neutral, PLANET_NVECS, SEGA_COLOR_GRAY );

   vec_planet_player = ALLOC( sizeof(vector_planet) );
   memcpy( vec_planet_player, vector_planet, sizeof(vector_planet) );
   colorize( vec_planet_player, PLANET_NVECS, SEGA_COLOR_GREEN );

   vec_planet_ai = ALLOC( sizeof(vector_planet) );
   memcpy( vec_planet_ai, vector_planet, sizeof(vector_planet) );
   colorize( vec_planet_ai, PLANET_NVECS, SEGA_COLOR_RED );

   // === selection cursor: a purple target reticle, not the planet itself ===
   vector_t* vec_target = ALLOC( sizeof(vector_target) );
   memcpy( vec_target, vector_target, sizeof(vector_target) );
   colorize( vec_target, sizeof(vector_target)/sizeof(vector_t), SEGA_COLOR_MAGENTA );

   // === lay out the planets: random positions, rejecting spots too close to
   //     any planet already placed, so none of them overlap ===
   for ( uint8_t i = 0; i < NUM_PLANETS; i++ ) {
      planet_t* p = &planets[i];
      p->scale  = TIER_SCALE[ PLANET_TIER[i] ];
      p->growth = MAX( 1, p->scale >> 7 );
      p->value  = 0;
      p->owner  = OWNER_NEUTRAL;

      uint16_t x = CENTER_X, y = CENTER_Y;
      for ( uint8_t attempt = 0; attempt < MAX_PLACEMENT_ATTEMPTS; attempt++ ) {
         do { x = (MIN_X+PLANET_MARGIN) + (rand16() & 0x03FF); } while ( x > (MAX_X-PLANET_MARGIN) );
         do { y = (MIN_Y+PLANET_MARGIN) + (rand16() & 0x03FF); } while ( y > (MAX_Y-PLANET_MARGIN) );

         bool ok = true;
         for ( uint8_t j = 0; j < i; j++ ) {
            int16_t ddx = (int16_t)x - (int16_t)planets[j].x;
            int16_t ddy = (int16_t)y - (int16_t)planets[j].y;
            if ( ABS(ddx) < MIN_SEPARATION && ABS(ddy) < MIN_SEPARATION ) { ok = false; break; }
         }
         if ( ok ) break; // good spot, stop trying
      }
      p->x = x;
      p->y = y;

      uint16_t rotation = (uint16_t)rand8() << 2; // random facing, just for visual variety
      drawSymbol( sym_planet[i], vec_planet_neutral, p->x, p->y, rotation, p->scale );

      const char s[] = "88";
      p->vec_label = ALLOC( measureString(s) * sizeof(vector_t) );
      updatePlanetLabel(i);
   }
   buildCursorOrder();

   // === give the player and the AI each a starting planet ===
   uint8_t player_start = claimRandomPlanet( OWNER_PLAYER, vec_planet_player, 0xFF );
   uint8_t player_owned_count = 1;
   claimRandomPlanet( OWNER_AI, vec_planet_ai, player_start );

   uint8_t selected_ix = nextSelectable( player_start );
   drawSymbol( sym_cursor, vec_target, planets[selected_ix].x, planets[selected_ix].y,
               SEGA_ANGLE(0), planets[selected_ix].scale );

   // === fleet art: one shared ship shape per side, but each fleet slot needs its own label ===
   vector_t* vec_ship_player = ALLOC( sizeof(vector_ship) );
   memcpy( vec_ship_player, vector_ship, sizeof(vector_ship) );
   vector_t* vec_ship_ai = ALLOC( sizeof(vector_ship) );
   memcpy( vec_ship_ai, vector_ship, sizeof(vector_ship) );
   colorize( vec_ship_ai, sizeof(vector_ship)/sizeof(vector_t), SEGA_COLOR_RED );

   vector_t* vec_fleet_label[MAX_FLEETS];
   for ( uint8_t f = 0; f < MAX_FLEETS; f++ ) {
      const char s88[] = "88";
      vec_fleet_label[f] = ALLOC( measureString(s88) * sizeof(vector_t) );
      fleets[f].active = false;
      fleets[f].battling = false;
   }

   const char s1[] = "claim the galaxy";
   vec_string1 = ALLOC( measureString(s1) * sizeof(vector_t) );
   drawString( sym_string1, vec_string1, GAME_TEXT_X, GAME_TEXT_Y, GAME_TEXT_SIZE, SEGA_COLOR_YELLOW, s1 );
   delayOrButton(4000);
   sym_string1->visible = false;
   FREE( vec_string1 );

   spinner_vector_angle( SPIN_RESET );

   uint16_t growth_tick     = system_tick;
   uint8_t  growth_ix       = 0;     // which planet gets its turn next
   uint8_t  growth_lap      = 0;     // counts full passes through all planets
   uint16_t ai_think_tick   = system_tick;
   uint16_t blink_tick      = system_tick;
   bool     blink_on        = false;
   uint8_t  last_selected_ix = selected_ix;
   uint16_t last_angle      = 0;
   int16_t  spin_accum      = 0;
   uint8_t  prev_buttons    = 0;
   bool     player_won      = false;

   while ( true ) {

      waitAnimate(0);
      drawScore( score, false );

      // --- growth: one planet at a time, cycling through all of them once per
      //     half-second; unowned (neutral) planets only grow on every other lap ---
      if ( system_tick - growth_tick > GROWTH_STEP_INTERVAL ) {
         growth_tick = system_tick;

         uint8_t i = growth_ix;
         growth_ix = (growth_ix + 1 == NUM_PLANETS) ? 0 : growth_ix + 1;
         if ( growth_ix == 0 ) growth_lap++;

         if ( planets[i].owner != OWNER_NEUTRAL || (growth_lap & 1) ) {
            uint8_t nv = planets[i].value + planets[i].growth;
            if ( nv > PLANET_VALUE_MAX ) nv = PLANET_VALUE_MAX;
            if ( nv != planets[i].value ) {
               planets[i].value = nv;
               updatePlanetLabel(i);
            }
         }
      }

      // --- spinner: step selection to the next/previous planet ---
      {
         uint16_t angle = spinner_vector_angle( SPIN_NORMAL );
         int16_t delta = (int16_t)(angle - last_angle);
         if ( delta > 512 )  delta -= 1024;
         if ( delta < -512 ) delta += 1024;
         last_angle = angle;

         spin_accum += delta;
         while ( spin_accum >= DETENT_SIZE ) {
            spin_accum -= DETENT_SIZE;
            selected_ix = nextSelectable( selected_ix );
         }
         while ( spin_accum <= -DETENT_SIZE ) {
            spin_accum += DETENT_SIZE;
            selected_ix = prevSelectable( selected_ix );
         }
      }

      // --- selection cursor: a purple reticle that follows the selection, and blinks ---
      if ( selected_ix != last_selected_ix ) {
         last_selected_ix = selected_ix;
         drawSymbol( sym_cursor, vec_target, planets[selected_ix].x, planets[selected_ix].y,
                     SEGA_ANGLE(0), planets[selected_ix].scale );
         sym_cursor->visible = true; // force visible on the new planet, even mid-blink-off
         blink_on = true;
         blink_tick = system_tick;
      }
      if ( system_tick - blink_tick > 15 ) {
         blink_tick = system_tick;
         blink_on = !blink_on;
         sym_cursor->visible = blink_on;
      }

      // --- fire: every planet the player owns (other than the target) launches its own fleet ---
      uint8_t buttons = PORT_374;
      if ( (buttons & BUTTON_FIRE) && !(prev_buttons & BUTTON_FIRE) ) {
         for ( uint8_t src = 0; src < NUM_PLANETS; src++ ) {
            if ( src == selected_ix ) continue;
            if ( planets[src].owner != OWNER_PLAYER || planets[src].value == 0 ) continue;
            launchFleet( src, selected_ix, OWNER_PLAYER, vec_ship_player, vec_fleet_label );
         }
      }
      prev_buttons = buttons;

      // --- the AI: periodically, its strongest planet attacks a random
      //     planet it doesn't already own ---
      if ( system_tick - ai_think_tick > AI_THINK_INTERVAL ) {
         ai_think_tick = system_tick;

         uint8_t best_src = 0xFF;
         uint8_t best_value = AI_MIN_ATTACK_SHIPS - 1;
         for ( uint8_t i = 0; i < NUM_PLANETS; i++ ) {
            if ( planets[i].owner == OWNER_AI && planets[i].value > best_value ) {
               best_value = planets[i].value;
               best_src = i;
            }
         }

         if ( best_src != 0xFF ) {
            uint8_t target = rand8() & 0x0F;
            uint8_t tries = 0;
            while ( (target >= NUM_PLANETS || planets[target].owner == OWNER_AI) && tries < 32 ) {
               target = rand8() & 0x0F;
               tries++;
            }
            if ( tries < 32 ) {
               launchFleet( best_src, target, OWNER_AI, vec_ship_ai, vec_fleet_label );
            }
         }
      }

      // --- fleets in flight: periodically re-aim at the target, bounding how
      //     far the low-speed integer rounding can drift off course ---
      for ( uint8_t f = 0; f < MAX_FLEETS; f++ ) {
         if ( !fleets[f].active || fleets[f].battling ) continue;
         if ( system_tick - fleets[f].retarget_tick <= REAIM_INTERVAL ) continue;
         fleets[f].retarget_tick = system_tick;

         int16_t dx = (int16_t)fleets[f].park_x - (int16_t)sym_fleet[f]->x;
         int16_t dy = (int16_t)fleets[f].park_y - (int16_t)sym_fleet[f]->y;
         uint16_t heading = deltaToVector( dx, dy );
         sym_fleet[f]->rotation = heading;
         setTrajectory( SID(sym_fleet[f]), FLEET_SPEED, heading );
         setTrajectory( SID(sym_fleet_label[f]), FLEET_SPEED, heading );
      }

      // --- fleets arrive: dock at the target, then either reinforce or start
      //     fighting it -- independently of any other fleet already there ---
      for ( uint8_t f = 0; f < MAX_FLEETS; f++ ) {
         if ( !fleets[f].active || fleets[f].battling ) continue;
         {
            int16_t dx = (int16_t)fleets[f].park_x - (int16_t)sym_fleet[f]->x;
            int16_t dy = (int16_t)fleets[f].park_y - (int16_t)sym_fleet[f]->y;
            if ( ABS(dx) >= PARK_THRESHOLD || ABS(dy) >= PARK_THRESHOLD ) continue;
         }

         setStop( SID(sym_fleet[f]) );
         setStop( SID(sym_fleet_label[f]) );

         planet_t* target = &planets[ fleets[f].target ];

         if ( target->owner == fleets[f].owner ) {
            // reinforcing a planet its own side already owns: no fight, ships just join
            uint8_t nv = target->value + fleets[f].ships;
            target->value = MIN( nv, PLANET_VALUE_MAX );
            updatePlanetLabel( fleets[f].target );
            sym_fleet[f]->visible = false;
            sym_fleet_label[f]->visible = false;
            fleets[f].active = false;
            continue;
         }

         // parked at the target and fighting -- stays visible until this
         // fleet's own fight concludes (captured, repelled, or the target
         // gets captured out from under it by another fleet), below. every
         // fleet attacking the same target shares its battle_tick, so they
         // all land hits on the same beat instead of their own offset timers
         bool already_fighting = false;
         for ( uint8_t g = 0; g < MAX_FLEETS; g++ ) {
            if ( g == f || !fleets[g].active || !fleets[g].battling ) continue;
            if ( fleets[g].target == fleets[f].target ) { already_fighting = true; break; }
         }
         if ( !already_fighting ) target->battle_tick = system_tick;
         fleets[f].battling = true;
      }

      // --- the fights: every attacking fleet trades hits with the target on its
      //     own schedule, independently, so several can be fighting it at once ---
      for ( uint8_t f = 0; f < MAX_FLEETS; f++ ) {
         if ( !fleets[f].active || !fleets[f].battling ) continue;

         planet_t* target = &planets[ fleets[f].target ];

         // another fleet may have already captured this target since we
         // started fighting it -- fold in as a reinforcement instead of
         // continuing an already-decided fight
         if ( target->owner == fleets[f].owner ) {
            uint8_t nv = target->value + fleets[f].ships;
            target->value = MIN( nv, PLANET_VALUE_MAX );
            updatePlanetLabel( fleets[f].target );
            sym_fleet[f]->visible = false;
            sym_fleet_label[f]->visible = false;
            fleets[f].active = false;
            fleets[f].battling = false;
            continue;
         }

         if ( system_tick - target->battle_tick <= EXCHANGE_INTERVAL ) continue;
         target->battle_tick = system_tick; // shared by every fleet attacking this target

         uint8_t atk_dmg = HIT_MIN + (rand8() & HIT_MASK);
         uint8_t def_dmg = HIT_MIN + (rand8() & HIT_MASK);

         target->value  = (target->value > atk_dmg)   ? target->value - atk_dmg   : 0;
         fleets[f].ships = (fleets[f].ships > def_dmg) ? fleets[f].ships - def_dmg : 0;
         updatePlanetLabel( fleets[f].target );
         updateFleetLabel( f, vec_fleet_label );
         SOUND_COMMAND = PHASER;

         if ( target->value == 0 ) {
            // captured: surviving ships become the new garrison
            if ( target->owner == OWNER_PLAYER ) player_owned_count--; // player loses this one
            target->owner = fleets[f].owner;
            if ( target->owner == OWNER_PLAYER ) player_owned_count++; // player gains this one
            target->value = MAX( fleets[f].ships, 1 );
            sym_planet[fleets[f].target]->vector_addr = (uint16_t)planetVec( fleets[f].target );
            updatePlanetLabel( fleets[f].target );
            if ( fleets[f].owner == OWNER_PLAYER ) score += target->value + 5;
            SOUND_COMMAND = DOCK;
            sym_fleet[f]->visible = false;
            sym_fleet_label[f]->visible = false;
            fleets[f].active = false;
            fleets[f].battling = false;
         } else if ( fleets[f].ships == 0 ) {
            // repelled: planet keeps whatever strength it has left, ownership unchanged
            SOUND_COMMAND = SHIELD_HIT;
            sym_fleet[f]->visible = false;
            sym_fleet_label[f]->visible = false;
            fleets[f].active = false;
            fleets[f].battling = false;
         }
      }

      // --- win/loss: conquer everything, or be wiped out with nothing left in flight ---
      if ( player_owned_count >= NUM_PLANETS ) {
         player_won = true;
         break;
      }
      if ( player_owned_count == 0 ) {
         bool player_has_fleet = false;
         for ( uint8_t f = 0; f < MAX_FLEETS; f++ ) {
            if ( fleets[f].active && fleets[f].owner == OWNER_PLAYER ) { player_has_fleet = true; break; }
         }
         if ( !player_has_fleet ) break; // eliminated
      }
   }

   // === victory or defeat ===
   if ( player_won ) {
      const char s[] = "galaxy conquered";
      vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );
      drawString( sym_string1, vec_string1, CENTER_X-260, GAME_TEXT_Y, GAME_TEXT_SIZE, SEGA_COLOR_GREEN, s );
   } else {
      const char s[] = "eliminated";
      vec_string1 = ALLOC( measureString(s) * sizeof(vector_t) );
      drawString( sym_string1, vec_string1, CENTER_X-170, GAME_TEXT_Y, GAME_TEXT_SIZE, SEGA_COLOR_RED, s );
   }
   waitAnimate( SECONDS(3) );
   sym_string1->visible = false;
   FREE( vec_string1 );

   // === cleanup ===
   for ( uint8_t i = 0; i < NUM_PLANETS; i++ ) {
      sym_planet[i]->visible = false;
      sym_planet_label[i]->visible = false;
      FREE( planets[i].vec_label );
   }
   for ( uint8_t f = 0; f < MAX_FLEETS; f++ ) {
      sym_fleet[f]->visible = false;
      sym_fleet_label[f]->visible = false;
      FREE( vec_fleet_label[f] );
   }
   sym_cursor->visible = false;
   FREE( vec_ship_player );
   FREE( vec_ship_ai );
   FREE( vec_target );
   FREE( vec_planet_neutral );
   FREE( vec_planet_player );
   FREE( vec_planet_ai );
}
