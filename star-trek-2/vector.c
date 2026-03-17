////////////////////////////////////
#include "sega.h"


void initVector( vector_t *const vec, uint8_t size, uint16_t angle, uint8_t color ) {
   memset( (uint8_t*)vec, 0x00, sizeof(vector_t) );
   vec->color = color;
   vec->size = size;
   vec->angle = angle;
}

void initSymbol( symbol_t *const sym, vector_t *const vec ) {
   memset( (uint8_t*)sym, 0x00, sizeof(symbol_t) );
   sym->x = CENTER_X; 
   sym->y = CENTER_Y;
   sym->vector_addr = (uint16_t)vec;
   sym->rotation = SEGA_ANGLE(0);
   sym->scale = 0x80; // 1x
}

void drawSymbol( symbol_t *const sym, vector_t *const vec, uint16_t x, uint16_t y, uint16_t sega_angle, uint8_t scale ) {
   sym->x = x;
   sym->y = y;
   sym->vector_addr = (uint16_t)vec;
   sym->rotation = sega_angle;
   sym->scale = scale;
   sym->visible = true;
}

void resetSymbol( symbol_t *const sym, uint16_t x, uint16_t y, uint16_t sega_angle, uint8_t scale ) {
   sym->x = x;
   sym->y = y;
   sym->rotation = sega_angle;
   if ( scale != 0 ) sym->scale = scale;
}

void enableSymbol( symbol_t *const sym, uint16_t x, uint16_t y, uint16_t sega_angle, uint8_t scale ) {
   resetSymbol( sym, x, y, sega_angle, scale );
   sym->visible = true;
}

bool checkColission( symbol_t *const s0, symbol_t *const s1 ) {
   uint16_t x0 = s0->x-(HITBOX_SZ/2);
   uint16_t x1 = s0->x+(HITBOX_SZ/2);
   uint16_t y0 = s0->y-(HITBOX_SZ/2);
   uint16_t y1 = s0->y+(HITBOX_SZ/2);
   return ( s1->x > x0 && s1->x < x1 && s1->y > y0 && s1->y < y1 );
}


typedef struct {
   int8_t x_speed;
   int8_t y_speed;
   int8_t rotation_speed;
   int8_t resize_speed;
   bool slow; // floating point speeds are tempting
} motion_t;

static motion_t motion[MAX_SYMBOLS] = { {0,}, };

void resetAnimate(void) {   
   memset( (uint8_t*)motion, 0x00, sizeof(motion) );
}

void animate(uint16_t system_tick, symbol_t* symbols, uint8_t symbol_count) {
   static uint16_t last_tick = 0;
   uint8_t frame = system_tick - last_tick;

   if ( frame == 0 ) return;
   last_tick = system_tick;

   for (uint8_t i=0; i<MIN(symbol_count,MAX_SYMBOLS); i++) {
      symbol_t *const sym = &symbols[i];
      if ( sym->visible ) {
         motion_t *m = &motion[ i ];
         if ( m->slow && ((system_tick & 0x0003) != 0) ) continue;
         sym->rotation = (sym->rotation + m->rotation_speed) & 0x03FF;
         uint8_t last_scale = sym->scale;
         if ( m->resize_speed ) {
            sym->scale += m->resize_speed;
            if ( m->resize_speed > 0 && sym->scale < last_scale ) {
               sym->visible = false;
            }
            if ( m->resize_speed < 0 && sym->scale > last_scale ) {
               sym->visible = false;
            }
         }
         if ( m->x_speed ) {
            sym->x += m->x_speed;
         }
         if ( m->y_speed ) {
            sym->y += m->y_speed;
         }
      }
   }
}

void setTrajectory( uint8_t sid, uint8_t velocity, uint16_t sega_angle ) {
   motion_t *m = &motion[ sid ];
   int16_t x, y;
   vectorToXY( sega_angle, velocity, &x, &y );
   m->x_speed = x;
   m->y_speed = y;
}

void setSpeeds( uint8_t sid, int8_t x_speed, int8_t y_speed ) {
   motion_t *m = &motion[ sid ];
   m->x_speed = x_speed;
   m->y_speed = y_speed;
}

void setRotationSpeed( uint8_t sid, int8_t rotation_speed ) {
   motion_t *m = &motion[ sid ];
   m->rotation_speed = rotation_speed;
}

void setResizeSpeed( uint8_t sid, int8_t resize_speed ) {
   motion_t *m = &motion[ sid ];
   m->resize_speed = resize_speed;
}

void setStop( uint8_t sid ) {
   motion_t *m = &motion[ sid ];
   memset( (uint8_t*)m, 0x00, sizeof(motion_t) );
}
