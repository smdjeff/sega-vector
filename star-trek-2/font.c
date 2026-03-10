////////////////////////////////////
#include "sega.h"



const uint8_t font_data_0[] = {
     #include "art/0.h"
};
const uint8_t font_data_1[] = {
     #include "art/1.h"
};
const uint8_t font_data_2[] = {
     #include "art/2.h"
};
const uint8_t font_data_3[] = {
     #include "art/3.h"
};
const uint8_t font_data_4[] = {
     #include "art/4.h"
};
const uint8_t font_data_5[] = {
     #include "art/5.h"
};
const uint8_t font_data_6[] = {
     #include "art/6.h"
};
const uint8_t font_data_7[] = {
     #include "art/7.h"
};
const uint8_t font_data_8[] = {
     #include "art/8.h"
};
const uint8_t font_data_9[] = {
     #include "art/9.h"
};

const uint8_t font_data_a[] = {
     #include "art/a.h"
};
const uint8_t font_data_b[] = {
     #include "art/b.h"
};
const uint8_t font_data_c[] = {
     #include "art/c.h"
};
const uint8_t font_data_d[] = {
     #include "art/d.h"
};
const uint8_t font_data_e[] = {
     #include "art/e.h"
};
const uint8_t font_data_f[] = {
     #include "art/f.h"
};
const uint8_t font_data_g[] = {
     #include "art/g.h"
};
const uint8_t font_data_h[] = {
     #include "art/h.h"
};
const uint8_t font_data_i[] = {
     #include "art/i.h"
};
const uint8_t font_data_j[] = {
     #include "art/j.h"
};
const uint8_t font_data_k[] = {
     #include "art/k.h"
};
const uint8_t font_data_l[] = {
     #include "art/l.h"
};
const uint8_t font_data_m[] = {
     #include "art/m.h"
};
const uint8_t font_data_n[] = {
     #include "art/n.h"
};
const uint8_t font_data_o[] = {
     #include "art/o.h"
};
const uint8_t font_data_p[] = {
     #include "art/p.h"
};
const uint8_t font_data_q[] = {
     #include "art/q.h"
};
const uint8_t font_data_r[] = {
     #include "art/r.h"
};
const uint8_t font_data_s[] = {
     #include "art/s.h"
};
const uint8_t font_data_t[] = {
     #include "art/t.h"
};
const uint8_t font_data_u[] = {
     #include "art/u.h"
};
const uint8_t font_data_v[] = {
     #include "art/v.h"
};
const uint8_t font_data_w[] = {
     #include "art/w.h"
};
const uint8_t font_data_x[] = {
     #include "art/x.h"
};
const uint8_t font_data_y[] = {
     #include "art/y.h"
};
const uint8_t font_data_z[] = {
     #include "art/z.h"
};

typedef struct {
    const vector_t *data;
    uint8_t count;
    uint8_t ox;
    uint8_t oy;
} font_entry_t;

const font_entry_t font_lookup[] = {
    { (vector_t*)font_data_0, sizeof(font_data_0)/4, 35, 0 },
    { (vector_t*)font_data_1, sizeof(font_data_1)/4, 35, 0 },
    { (vector_t*)font_data_2, sizeof(font_data_2)/4, 35, 0 },
    { (vector_t*)font_data_3, sizeof(font_data_3)/4, 35, 0 },
    { (vector_t*)font_data_4, sizeof(font_data_4)/4, 35, 0 },
    { (vector_t*)font_data_5, sizeof(font_data_5)/4, 35, 0 },
    { (vector_t*)font_data_6, sizeof(font_data_6)/4, 35, 0 },
    { (vector_t*)font_data_7, sizeof(font_data_7)/4, 35, 0 },
    { (vector_t*)font_data_8, sizeof(font_data_8)/4, 35, 0 },
    { (vector_t*)font_data_9, sizeof(font_data_9)/4, 35, 0 },

    { (vector_t*)font_data_a, sizeof(font_data_a)/4, 35, 0 },
    { (vector_t*)font_data_b, sizeof(font_data_b)/4, 35, 0 },
    { (vector_t*)font_data_c, sizeof(font_data_c)/4, 35, 0 },
    { (vector_t*)font_data_d, sizeof(font_data_d)/4, 35, 0 },
    { (vector_t*)font_data_e, sizeof(font_data_e)/4, 35, 0 },
    { (vector_t*)font_data_f, sizeof(font_data_f)/4, 35, 0 },
    { (vector_t*)font_data_g, sizeof(font_data_g)/4, 35, 0 },
    { (vector_t*)font_data_h, sizeof(font_data_h)/4, 35, 0 },
    { (vector_t*)font_data_i, sizeof(font_data_i)/4, 35, 0 },
    { (vector_t*)font_data_j, sizeof(font_data_j)/4, 35, 0 },
    { (vector_t*)font_data_k, sizeof(font_data_k)/4, 35, 0 },
    { (vector_t*)font_data_l, sizeof(font_data_l)/4, 35, 0 },
    { (vector_t*)font_data_m, sizeof(font_data_m)/4, 35, 0 },
    { (vector_t*)font_data_n, sizeof(font_data_n)/4, 35, 0 },
    { (vector_t*)font_data_o, sizeof(font_data_o)/4, 35, 0 },
    { (vector_t*)font_data_p, sizeof(font_data_p)/4, 35, 0 },
    { (vector_t*)font_data_q, sizeof(font_data_q)/4, 35, 0 },
    { (vector_t*)font_data_r, sizeof(font_data_r)/4, 35, 0 },
    { (vector_t*)font_data_s, sizeof(font_data_s)/4, 35, 0 },
    { (vector_t*)font_data_t, sizeof(font_data_t)/4, 35, 0 },
    { (vector_t*)font_data_u, sizeof(font_data_u)/4, 35, 0 },
    { (vector_t*)font_data_v, sizeof(font_data_v)/4, 35, 0 },
    { (vector_t*)font_data_w, sizeof(font_data_w)/4, 35, 0 },
    { (vector_t*)font_data_x, sizeof(font_data_x)/4, 35, 0 },
    { (vector_t*)font_data_y, sizeof(font_data_y)/4, 35, 0 },
    { (vector_t*)font_data_z, sizeof(font_data_z)/4, 35, 0 },
    { NULL,                   0,                     35, 0 } // space
};

void colorize(vector_t *vec, uint16_t len, uint8_t color) {
   if (len == 0) return;
   uint8_t const masked_color = color & 0x7E;
   while (len--) { // fast z80 flag checking after decrement
      vec->color = (vec->color & ~0x7E) | masked_color;
      vec++; 
   }
}

void hex16( char *s, uint16_t value) {
   const char *hex = "0123456789abcdef";
   s[0] = hex[ (value >> 12) & 0x0F ];
   s[1] = hex[ (value >> 8)  & 0x0F ];
   s[2] = hex[ (value >> 4)  & 0x0F ];
   s[3] = hex[ (value >> 0)  & 0x0F ];
}

void dec2(char *s, uint8_t value) {
   uint8_t tens = 0;
   while (value >= 10) {
      value -= 10;
      tens++;
   }
   s[0] = '0' + tens;
   s[1] = '0' + value;
}

void dec4(char *s, uint16_t value) {
   uint8_t thousands = 0;
   uint8_t hundreds = 0;
   while (value >= 1000) {
      value -= 1000;
      thousands++;
   }
   s[0] = '0' + thousands;
   while (value >= 100) {
      value -= 100;
      hundreds++;
   }
   s[1] = '0' + hundreds;
   uint8_t rem = (uint8_t)value;
   uint8_t tens = 0;
   while (rem >= 10) {
      rem -= 10;
      tens++;
   }
   s[2] = '0' + tens;
   s[3] = '0' + rem;
   for (uint8_t i=0;i<3;i++) {
     if ( s[i] == '0' ) s[i] = ' '; else break;
   }
}

#define VEC_OFF_Y(oy) ((uint32_t)(oy) << 8 | (uint32_t)SEGA_ANGLE(0)  << 16)
#define VEC_OFF_X(ox) ((uint32_t)(ox) << 8 | (uint32_t)SEGA_ANGLE(90) << 16)

void drawString(symbol_t *sym, vector_t *vec, uint16_t x, uint16_t y, uint8_t scale, uint8_t color, const char *str) {
    register vector_t *v_ptr = vec;

    while (*str) {
        uint8_t c = *str++;
        uint8_t idx;

        if (c >= 'a' && c <= 'z')      idx = c - 'a' + 10;
        else if (c >= '0' && c <= '9') idx = c - '0';
        else if (c == ' ')             idx = 36;
        else continue;

        const font_entry_t *e = &font_lookup[idx];

        if (e->data) {
            memcpy(v_ptr, e->data, e->count * sizeof(vector_t));
            v_ptr += e->count;
            (v_ptr - 1)->last = 0;
        }

        if (e->oy) {
            *(uint32_t*)v_ptr = VEC_OFF_Y(e->oy);
            v_ptr++;
        }

        if (e->ox) {
            *(uint32_t*)v_ptr = VEC_OFF_X(e->ox);
            v_ptr++;
        }
    }

    if (v_ptr > vec) (v_ptr - 1)->last = 1;

    if (color) {
        colorize(vec, (uint16_t)(v_ptr - vec), color);
    }

    sym->x = x;
    sym->y = y;
    sym->vector_addr = (uint16_t)vec;
    sym->scale = scale;
    sym->visible = 1;
}


uint16_t measureString(const char *str) __z88dk_fastcall {
    register uint16_t total = 0;
    
    while (*str) {
        uint8_t c = *str++;
        uint8_t idx;
        
        if (c >= 'a')      idx = c - 'a' + 10;
        else if (c >= '0') idx = c - '0';
        else if (c == ' ') idx = 36;
        else continue;

        const font_entry_t *e = &font_lookup[idx];
        
        total += e->count;
        if (e->oy) total++;
        if (e->ox) total++;
    }
    return total;
}





