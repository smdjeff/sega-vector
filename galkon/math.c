////////////////////////////////////
#include "sega.h"

 
uint8_t rand8(void) {
    static uint8_t a = 0, b = 0, c = 0, x = 0;
    x++;
    a = (a ^ c ^ x);
    b = (b + a);
    c = (c + (b >> 1) ^ a);
    return c;
}

uint16_t rand16(void) {
    return ((uint16_t)rand8()<<8) | (uint16_t)rand8();
}

// rand8()'s internal state always starts at 0, so a fresh boot replays the exact
// same sequence every time. Call this once with an unpredictable value (e.g. the
// tick count at the moment the player presses a button) to perturb it for real.
void seedRand(uint8_t n) {
    while (n--) rand8();
}

inline uint16_t xy_multiply( uint8_t x, uint8_t y ) {
   XY_MULTIPLICAND = x;
   XY_MULTIPLIER = y;
   uint16_t product = XY_MULTIPLIER;
   product += (uint16_t)XY_MULTIPLIER << 8;
   return product;
}

static const uint8_t recip_table[127] = {
   255,128,85,64,51,43,37,32,29,26,24,22,20,19,17,16,15,15,14,13,13,12,12,11,
   11,10,10,10,9,9,9,8,8,8,8,8,7,7,7,7,7,7,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,
   5,5,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,
   3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3
};

uint16_t div_16(uint16_t u, uint8_t v) {
    if (v == 0) return 0xFFFF; // div by zero
    uint8_t vr = 2;
    if (v <  128) vr = recip_table[v - 1];
    if (v == 255) vr = 1;
    return xy_multiply(u >> 8, vr) + (xy_multiply(u & 0xFF, vr) >> 8);
}

uint8_t divideBy10(uint8_t *value) {
    uint8_t count = 0;
    while (*value >= 10) {
        *value -= 10;
        count++;
    }
    return count;
}

uint8_t divideBy100(uint8_t *value) {
    uint8_t count = 0;
    while (*value >= 100) {
        *value -= 100;
        count++;
    }
    return count;
}


static const uint8_t sin_table[262] = {
   0,2,3,5,6,8,9,11,13,14,16,17,19,20,22,23,25,27,28,30,31,33,34,36,38,39,41,42,44,45,47,48,50,51,53,54,56,58,59,61,62,64,65,
   67,68,70,71,73,74,76,77,79,80,82,83,85,86,88,89,91,92,93,95,96,98,99,101,102,104,105,106,108,109,111,112,114,115,116,118,
   119,120,122,123,125,126,127,129,130,131,133,134,135,137,138,139,141,142,143,145,146,147,148,150,151,152,153,155,156,157,
   158,160,161,162,163,165,166,167,168,169,170,172,173,174,175,176,177,178,180,181,182,183,184,185,186,187,188,189,190,191,
   192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,211,212,213,214,215,216,217,217,218,219,
   220,221,221,222,223,224,224,225,226,227,227,228,229,229,230,231,231,232,233,233,234,235,235,236,236,237,238,238,239,239,
   240,240,241,241,242,242,243,243,244,244,245,245,246,246,246,247,247,248,248,248,249,249,249,250,250,250,251,251,251,251,
   252,252,252,252,253,253,253,253,253,254,254,254,254,254,254,254,254,255,255,255,255,255,255,255,255,255,
   255,255,255,255,255,255,255
};

// Ed Logg notes in his Asteroids code: SIN(PI+A) = -SIN(A) and COS(A) = SIN(A+PI/2)
// the table then is just 0-90 degrees.
// 90-180 is simply a reflection of 0-90
// 180-360 is a negative sign reflection of 0-180
// and cos is just sin rotated 90
// the table is large because it's using sega's 10bit angle datatype units unscaled for speed

static int8_t sinlut(uint16_t sega_angle, bool *negsign) {
    if ( sega_angle < SEGA_ANGLE(90) ) {
        *negsign = false;
        return sin_table[ sega_angle ];
    }
    if ( sega_angle < SEGA_ANGLE(180) ) {
        sega_angle -= SEGA_ANGLE(90);
        *negsign = false;
        return sin_table[ SEGA_ANGLE(90) - sega_angle ];
    }
    if ( sega_angle < SEGA_ANGLE(270) ) {
        sega_angle -= SEGA_ANGLE(180);
        *negsign = true;
        return sin_table[ sega_angle ];
    }
    sega_angle -= SEGA_ANGLE(270);
    *negsign = true;
    return sin_table[ SEGA_ANGLE(90) - sega_angle ];
}

static uint8_t coslut(uint16_t sega_angle, bool *negsign) {
    sega_angle += SEGA_ANGLE(90);
    if (sega_angle>=SEGA_ANGLE(360)) {
        sega_angle -= SEGA_ANGLE(360);
    }
    return sinlut( sega_angle, negsign );
}

void vectorToXY( uint16_t sega_angle, uint16_t length, int16_t *x, int16_t *y ) {
  bool sin_neg, cos_neg;
  
  sega_angle &= 0x03FF; // 0 to 2^10
  uint8_t sin_value = sinlut( sega_angle, &sin_neg );
  uint8_t cos_value = coslut( sega_angle, &cos_neg );

  *x = 0; *y = 0;
  do {
    uint16_t l = MIN(length,254);
    *x += xy_multiply( 1 + l, sin_value ) >> 8;
    *y += xy_multiply( 1 + l, cos_value ) >> 8;
    length -= l;
  } while (length);

  if (sin_neg) {
      *x = - *x;
  }
  if (cos_neg) {
      *y = - *y;
  }
}

// atan(ratio) for ratio 0..1 (one 45 degree octant), in SEGA angle units
// (1024 = 360deg, so this table spans 0-128). deltaToVector() below folds
// the other 7 octants onto this one via sign/mirror symmetry, the standard
// technique for a lookup-table atan2 -- dedicating the whole table to one
// wedge gives much finer resolution than a flat 2D x,y grid of the same size.
static const uint8_t atan_octant_lut[256] = {
   0, 1, 1, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10,
   10, 11, 11, 12, 13, 13, 14, 15, 15, 16, 16, 17, 18, 18, 19, 20,
   20, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 27, 28, 28, 29, 30,
   30, 31, 31, 32, 33, 33, 34, 34, 35, 36, 36, 37, 38, 38, 39, 39,
   40, 41, 41, 42, 42, 43, 44, 44, 45, 45, 46, 46, 47, 48, 48, 49,
   49, 50, 51, 51, 52, 52, 53, 53, 54, 55, 55, 56, 56, 57, 57, 58,
   58, 59, 60, 60, 61, 61, 62, 62, 63, 63, 64, 65, 65, 66, 66, 67,
   67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 74, 74, 75, 75,
   76, 76, 77, 77, 78, 78, 79, 79, 80, 80, 81, 81, 82, 82, 83, 83,
   84, 84, 84, 85, 85, 86, 86, 87, 87, 88, 88, 89, 89, 90, 90, 91,
   91, 91, 92, 92, 93, 93, 94, 94, 95, 95, 96, 96, 96, 97, 97, 98,
   98, 99, 99, 99, 100, 100, 101, 101, 102, 102, 102, 103, 103, 104, 104, 104,
   105, 105, 106, 106, 106, 107, 107, 108, 108, 108, 109, 109, 110, 110, 110, 111,
   111, 112, 112, 112, 113, 113, 113, 114, 114, 115, 115, 115, 116, 116, 116, 117,
   117, 118, 118, 118, 119, 119, 119, 120, 120, 120, 121, 121, 121, 122, 122, 122,
   123, 123, 123, 124, 124, 124, 125, 125, 125, 126, 126, 126, 127, 127, 127, 128,
};

// ratio (0-255, meaning 0.0-1.0) of small/big, exact to within 1 unit.
// div_16() is NOT precise enough for this: its reciprocal table only has real
// entries for v<128 and falls back to a flat, badly-wrong approximation for
// v>=128 (which is common here, since big is scaled to fill the 0-255 range).
// The hardware multiplier is exact, though, so binary-search the answer with it.
static uint8_t preciseRatio( uint8_t small, uint8_t big ) {
   if ( big == 0 ) return 0;
   uint16_t target = (uint16_t)small << 8;
   uint8_t lo = 0, hi = 255;
   while ( lo < hi ) {
      uint8_t mid = lo + ((hi - lo + 1) >> 1); // round up, avoids overflow
      if ( xy_multiply(big, mid) <= target ) lo = mid; else hi = mid - 1;
   }
   return lo;
}

// the angle of an arbitrary vector (dx,dy), in the same SEGA-angle convention
// vectorToXY() uses (0 = +y, 256 = +x, 512 = -y, 768 = -x -- a clockwise
// compass from "up"). works for any delta; unlike the old xyToVector() this
// is not anchored to screen center, and is not clamped to the visible area.
uint16_t deltaToVector( int16_t dx, int16_t dy ) {
   if ( dx == 0 && dy == 0 ) return 0;

   uint16_t adx = ABS(dx);
   uint16_t ady = ABS(dy);
   // scale down to fit the hardware multiplier's 8-bit inputs; only the ratio
   // between them matters here, so losing the low bits is fine. shift by only
   // as much as the larger of the two actually needs, to keep small deltas precise.
   uint16_t big = MAX(adx, ady);
   uint8_t shift = 0;
   while ( big > 255 ) { big >>= 1; shift++; }
   uint8_t sx = (uint8_t)(adx >> shift);
   uint8_t sy = (uint8_t)(ady >> shift);

   uint16_t ref; // 0-256: angle from the +y axis toward the +x axis, within one quadrant
   if ( sx <= sy ) {
      ref = atan_octant_lut[ preciseRatio(sx, sy) ];
   } else {
      ref = 256 - atan_octant_lut[ preciseRatio(sy, sx) ];
   }

   uint16_t angle;
   if ( dy >= 0 ) {
      angle = (dx >= 0) ? ref : (1024 - ref);
   } else {
      angle = (dx >= 0) ? (512 - ref) : (512 + ref);
   }
   return angle & 0x03FF;
}


