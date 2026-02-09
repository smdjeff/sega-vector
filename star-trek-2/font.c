////////////////////////////////////
#include "sega.h"



const uint8_t font_data_0[] = {
     #include "art/font/0.h"
};
const uint8_t font_data_1[] = {
     #include "art/font/1.h"
};
const uint8_t font_data_2[] = {
     #include "art/font/2.h"
};
const uint8_t font_data_3[] = {
     #include "art/font/3.h"
};
const uint8_t font_data_4[] = {
     #include "art/font/4.h"
};
const uint8_t font_data_5[] = {
     #include "art/font/5.h"
};
const uint8_t font_data_6[] = {
     #include "art/font/6.h"
};
const uint8_t font_data_7[] = {
     #include "art/font/7.h"
};
const uint8_t font_data_8[] = {
     #include "art/font/8.h"
};
const uint8_t font_data_9[] = {
     #include "art/font/9.h"
};

const uint8_t font_data_a[] = {
     #include "art/font/a.h"
};
const uint8_t font_data_b[] = {
     #include "art/font/b.h"
};
const uint8_t font_data_c[] = {
     #include "art/font/c.h"
};
const uint8_t font_data_d[] = {
     #include "art/font/d.h"
};
const uint8_t font_data_e[] = {
     #include "art/font/e.h"
};
const uint8_t font_data_f[] = {
     #include "art/font/f.h"
};
const uint8_t font_data_g[] = {
     #include "art/font/g.h"
};
const uint8_t font_data_h[] = {
     #include "art/font/h.h"
};
const uint8_t font_data_i[] = {
     #include "art/font/i.h"
};
const uint8_t font_data_j[] = {
     #include "art/font/j.h"
};
const uint8_t font_data_k[] = {
     #include "art/font/k.h"
};
const uint8_t font_data_l[] = {
     #include "art/font/l.h"
};
const uint8_t font_data_m[] = {
     #include "art/font/m.h"
};
const uint8_t font_data_n[] = {
     #include "art/font/n.h"
};
const uint8_t font_data_o[] = {
     #include "art/font/o.h"
};
const uint8_t font_data_p[] = {
     #include "art/font/p.h"
};
const uint8_t font_data_q[] = {
     #include "art/font/q.h"
};
const uint8_t font_data_r[] = {
     #include "art/font/r.h"
};
const uint8_t font_data_s[] = {
     #include "art/font/s.h"
};
const uint8_t font_data_t[] = {
     #include "art/font/t.h"
};
const uint8_t font_data_u[] = {
     #include "art/font/u.h"
};
const uint8_t font_data_v[] = {
     #include "art/font/v.h"
};
const uint8_t font_data_w[] = {
     #include "art/font/w.h"
};
const uint8_t font_data_x[] = {
     #include "art/font/x.h"
};
const uint8_t font_data_y[] = {
     #include "art/font/y.h"
};
const uint8_t font_data_z[] = {
     #include "art/font/z.h"
};

uint16_t fontAddress( char c ) {
   switch ( c ) {
        case '0': return (uint16_t)font_data_0;
        case '1': return (uint16_t)font_data_1;
        case '2': return (uint16_t)font_data_2;
        case '3': return (uint16_t)font_data_3;
        case '4': return (uint16_t)font_data_4;
        case '5': return (uint16_t)font_data_5;
        case '6': return (uint16_t)font_data_6;
        case '7': return (uint16_t)font_data_7;
        case '8': return (uint16_t)font_data_8;
        case '9': return (uint16_t)font_data_9;
        case 'a': return (uint16_t)font_data_a;
        case 'b': return (uint16_t)font_data_b;
        case 'c': return (uint16_t)font_data_c;
        case 'd': return (uint16_t)font_data_d;
        case 'e': return (uint16_t)font_data_e;
        case 'f': return (uint16_t)font_data_f;
        case 'g': return (uint16_t)font_data_g;
        case 'h': return (uint16_t)font_data_h;
        case 'i': return (uint16_t)font_data_i;
        case 'j': return (uint16_t)font_data_j;
        case 'k': return (uint16_t)font_data_k;
        case 'l': return (uint16_t)font_data_l;
        case 'm': return (uint16_t)font_data_m;
        case 'n': return (uint16_t)font_data_n;
        case 'o': return (uint16_t)font_data_o;
        case 'p': return (uint16_t)font_data_p;
        case 'q': return (uint16_t)font_data_q;
        case 'r': return (uint16_t)font_data_r;
        case 's': return (uint16_t)font_data_s;
        case 't': return (uint16_t)font_data_t;
        case 'u': return (uint16_t)font_data_u;
        case 'v': return (uint16_t)font_data_v;
        case 'w': return (uint16_t)font_data_w;
        case 'x': return (uint16_t)font_data_x;
        case 'y': return (uint16_t)font_data_y;
        case 'z': return (uint16_t)font_data_z;
   }
   return 0;
}

static uint16_t fontSize( char c ) {
   switch ( c ) {
        case '0': return sizeof(font_data_0);
        case '1': return sizeof(font_data_1);
        case '2': return sizeof(font_data_2);
        case '3': return sizeof(font_data_3);
        case '4': return sizeof(font_data_4);
        case '5': return sizeof(font_data_5);
        case '6': return sizeof(font_data_6);
        case '7': return sizeof(font_data_7);
        case '8': return sizeof(font_data_8);
        case '9': return sizeof(font_data_9);
        case 'a': return sizeof(font_data_a);
        case 'b': return sizeof(font_data_b);
        case 'c': return sizeof(font_data_c);
        case 'd': return sizeof(font_data_d);
        case 'e': return sizeof(font_data_e);
        case 'f': return sizeof(font_data_f);
        case 'g': return sizeof(font_data_g);
        case 'h': return sizeof(font_data_h);
        case 'i': return sizeof(font_data_i);
        case 'j': return sizeof(font_data_j);
        case 'k': return sizeof(font_data_k);
        case 'l': return sizeof(font_data_l);
        case 'm': return sizeof(font_data_m);
        case 'n': return sizeof(font_data_n);
        case 'o': return sizeof(font_data_o);
        case 'p': return sizeof(font_data_p);
        case 'q': return sizeof(font_data_q);
        case 'r': return sizeof(font_data_r);
        case 's': return sizeof(font_data_s);
        case 't': return sizeof(font_data_t);
        case 'u': return sizeof(font_data_u);
        case 'v': return sizeof(font_data_v);
        case 'w': return sizeof(font_data_w);
        case 'x': return sizeof(font_data_x);
        case 'y': return sizeof(font_data_y);
        case 'z': return sizeof(font_data_z);
   }
   return 0;
}

static uint8_t offset_x( char ch ) {
     return 35;
}

static uint8_t offset_y( char ch ) {
     return 0;
}

static uint16_t font_addr_string = 0;
uint16_t installFonts( uint16_t addr ) {
     font_addr_string = addr;
     return addr;
}

static void colorize( uint8_t *const vec, uint16_t len, uint8_t color ) {
   for (uint16_t i=0; i<len; i+=sizeof(vector_t)) {
      if ( (uint16_t)&vec[i] < VECTOR_RAM + SYMBOLS_SZ  ) kill(0xE3);
      if ( (uint16_t)&vec[i] + len >= VECTOR_RAM_END ) kill(0xE4);
      vec[i] &= ~0x7E;
      vec[i] |= (color & 0x7E);
   }
}

void drawString( symbol_t *const sym, uint16_t x, uint16_t y, uint8_t scale, uint8_t color, const char *const str, uint8_t len ) {
   uint8_t *vec = (uint8_t*)font_addr_string;
   uint16_t v_sz = 0;
   
   sym->vector_addr = V_ADDR(0); // blank

  for (uint8_t i=0; i<len; i++) {
      if ( (uint16_t)&vec[v_sz] + len + (sizeof(vector_t)*2) >= VECTOR_RAM_END ) kill(0xE1);

      char ch = str[ i ];
      uint8_t *in = (uint8_t*)fontAddress( ch );
      if ( in ) {
           uint16_t sz = fontSize( ch );
           memcpy( &vec[v_sz], in, sz );
           v_sz += sz;
           vec[v_sz - 4] &= ~ SEGA_LAST;
      }

     uint8_t oy = offset_y( ch );
     if ( oy ) {
          vec[v_sz++] = 0x00;
          vec[v_sz++] = oy;
          vec[v_sz++] = LSB(SEGA_ANGLE(0));
          vec[v_sz++] = MSB(SEGA_ANGLE(0));
     }

     uint8_t ox = offset_x( ch );
     if ( ox ) {
          vec[v_sz++] = 0x00;
          vec[v_sz++] = ox;
          vec[v_sz++] = LSB(SEGA_ANGLE(90));
          vec[v_sz++] = MSB(SEGA_ANGLE(90));
     }
   }

   if ( (uint16_t)&vec[v_sz-sizeof(vector_t)] < VECTOR_RAM + SYMBOLS_SZ ) kill(0xE2);

   vec[v_sz - sizeof(vector_t)] |= SEGA_LAST;

   if ( color ) {
        colorize( vec, v_sz, color );
   }

   sym->x = x;
   sym->y = y;
   sym->vector_addr = (uint16_t)vec;
   sym->scale = scale;
   sym->visible = 1;
}



