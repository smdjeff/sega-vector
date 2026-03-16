#ifndef _SEGA_H_
#define _SEGA_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>


#define BIT(n)             (1<<(n))
#define MIN(a,b)           (((a)<(b))?(a):(b))
#define MAX(a,b)           (((a)>(b))?(a):(b))
#define LSB(x)             (uint8_t)((uint16_t)(x) & 0xFF)
#define MSB(x)             (uint8_t)(((uint16_t)(x) >> 8) & 0xFF)
#define LE(x)              LSB(x), MSB(x)
#define SECONDS(s)         ((s)*1000/25)
#define ALLOC(sz)          _alloc_or_kill( heap_alloc( heap, sz ), __LINE__ )
#define FREE(p)            if (p) { heap_free( heap, p ); p=NULL; }
#define SID(s)             ((s)-symbols)
#define ABS(x)             ((x)<0?-(x):(x))


////////////////////////////////////
// sega g80 memory map
//
// 0x0000   -------------------
//          CPU BOOT ROM (2k)
// 0x0800   -------------------
//          CPU ROM board (46k)
// 0xC000   -------------------
//          NA (2k)
// 0xC800   -------------------
//          CPU RAM (2k)
// 0xD000   -------------------
//          USB RAM (4k)
// 0xE000   -------------------
//          XY RAM (4k)
//             symbols
//             vectors
// 0xF000   -------------------
//          NA (4k)
// 0xFFFF

#define CPU_ROM         (0x0000) // 2k power on and diagnostics rom (cpu board)
#define CPU_ROM_SZ      (2*1024)

#define ROM_BOARD       (0x0800) // 46k rom board
#define ROM_BOARD_SZ    (46*1024)

#define CPU_RAM         (0xC800) // 2k ram (cpu board)
#define CPU_RAM_SZ      (2*1024)

#define USB_RAM         (0xD000) // 4k ram for the 8035 (universal serial board)
#define USB_RAM_SZ      (4*1024)

#define VECTOR_RAM      (0xE000) // 4k ram (xy board)
#define VECTOR_RAM_SZ   (4*1024)
#define VECTOR_RAM_END  (VECTOR_RAM+VECTOR_RAM_SZ)
////////////////////////////////////

// Test button on CPU board asserts NMI
// 40Hz interrupt from vector timing board

#pragma output CRT_ORG_DATA = 0xC800  // start of RAM

#pragma output CRT_MODEL = 1 // data section copied from ROM into RAM on program start
//#pragma output CRT_MODEL = 2 // data section zx7 compressed in ROM, decompressed into RAM on program start

#ifdef ENABLE_BOOTROM
/*
   Simply setting CRT_ORG_VECTOR_TABLE will not work, since it's  ignored when CRT_ORG_CODE is non zero.
   Workaround is to patch z88dk/libsrc/_DEVELOPMENT/target/z80/startup/z80_crt_0.asm.m4 as follows:
   ;IF (ASMPC = 0) && (__crt_org_code = 0)
     include "../crt_page_zero_z80.inc"
   ;ENDIF
*/
   #pragma output CRT_ORG_CODE = 0x0800 // when using boot rom
//   #pragma output CRT_ORG_VECTOR_TABLE = -0x0800
#else
   #pragma output CRT_ORG_CODE = 0x0000
#endif


#pragma output REGISTER_SP = 0xCFFF

// used to calculate heap size (default is just unused ram)
// #pragma output CRT_STACK_SIZE = 0xf // 128

#pragma output CRT_ENABLE_RST = 0x80 // User implements rst38h (IM1). user must preserve register values and exit with "ei; reti"
// #pragma output CRT_ENABLE_RST = 0x00 // User implements no isrs (default)
#pragma output CRT_ENABLE_NMI = 1 // program supplies nmi isr
#pragma output CRT_ENABLE_EIDI = 0x01 // disable interrupts di on start
//#pragma output CRT_ENABLE_EIDI = 0x02 // enable interrupts ei on start
#pragma output CRT_INTERRUPT_MODE = 1
#pragma output CLIB_MALLOC_HEAP_SIZE = 0
#pragma output CLIB_STDIO_HEAP_SIZE = 0


__sfr __at 0xfc PORT_374;
// |   D7    |   D6    |   D5    |   D4    |   D3    |   D2    |   D1    |   D0   |
// +=========+=========+=========+=========+=========+=========+=========+========+
// |  P1-30  |  P1-29  |  P1-28  |  P1-27  |  P1-26  |  P1-25  |  P1-24  |  P1-23 |
// +---------+---------+---------+---------+---------+---------+---------+--------+
// |RotL P2  |RotR P2  | FIRE P2 |THRUST P2|   <--- SPACE FURY (cocktail)         |
// +---------+---------+---------+---------+---------+---------+---------+--------+
// ELIMINATOR --->                                   |RotR Red |ThrustRed|Fire Red|
// +---------+---------+---------+---------+---------+---------+---------+--------+
// STAR TREK  --->     | WARP    | PHOTON  | PHASER  | IMPULSE | 2Player |1Player 
#define BUTTON_PLAYER_1    BIT(0)
#define BUTTON_PLAYER_2    BIT(1)
#define BUTTON_THRUST      BIT(2)
#define BUTTON_FIRE        BIT(3)
#define BUTTON_PHOTON      BIT(4)
#define BUTTON_WARP        BIT(5)

__sfr __at 0xf8 PORT_370;
#define SELECT_SPINNER     0xFE
#define SELECT_BUTTONS     0xFF
#define IO_SERVICE_N       BIT(5)
#define IO_COIN0_N         BIT(6)
#define IO_COIN1_N         BIT(7)

__sfr __at 0xf9 PORT_371;

__sfr __at 0xbe XY_MULTIPLIER;
__sfr __at 0xbd XY_MULTIPLICAND;
__sfr __at 0xbf XY_INIT;

__sfr __at 0x38 SPEECH_COMMAND;
__sfr __at 0x3b SPEECH_CONTROL;
__sfr __at 0x39 VOTRAX_COMMAND;
__sfr __at 0x3f SOUND_COMMAND;


//   2048-   -------------------
//          |                   |
//          |                   |
//   1536-  |     ---------     |
//          |    |         |    |
//   1024-  |    |    +    |    |
//          |    |         |    |
//    512-  |     ---------     |
//          |                   |
//          |                   |
//      0-   -------------------
//          |    |    |    |    |
//          0   512 1024  1536 2048

//   315  0  45
//      \ | /
//   270 -+- 90
//      / | \
//   225 180  135

// 0 = 0 deg, 2^10 (1024) = 360 deg
#define SEGA_ANGLE(deg)    ((int16_t)(((float)(deg))*2.845))
#define SEGA_DEGREES(a)    ((uint16_t)divide2_9(a))

#define V_ADDR(x) (VECTOR_RAM+SYMBOLS_SZ+((x)*4))
#define S_ADDR(x) (VECTOR_RAM + ((x)*10))

typedef struct {
   union {
      struct {
         uint8_t visible : 1;  // lsb
         uint8_t group   : 6;  // user
         uint8_t last    : 1;  // msb
      };
      uint8_t flags;
   };
   uint16_t x;           // 0x000-0x7FF 10bit wrap, viewport is 0x200-0x600 with 0x400 center 
   uint16_t y;
   uint16_t vector_addr; // E000 - EFFF 4k Vector RAM
   uint16_t rotation;    // 0x7FF 10 bit angle of whole symbol
   uint8_t scale;        // 0x40 = 1/2, 0x80 = 1x, 0xff = 2x
} symbol_t; // 10 bytes

#define SFIELD_VISIBLE(row) (((row)*10)+0)
#define SFIELD_X_L(row)     (((row)*10)+1)
#define SFIELD_X_H(row)     (((row)*10)+2)
#define SFIELD_Y_L(row)     (((row)*10)+3)
#define SFIELD_Y_H(row)     (((row)*10)+4)
#define SFIELD_ADDR_L(row)  (((row)*10)+5)
#define SFIELD_ADDR_H(row)  (((row)*10)+6)
#define SFIELD_ANGLE_L(row) (((row)*10)+7)
#define SFIELD_ANGLE_H(row) (((row)*10)+8)
#define SFIELD_SCALE(row)   (((row)*10)+9)

typedef struct {
   union {
      struct {
         uint8_t visible : 1; // lsb
         uint8_t blue    : 2;
         uint8_t green   : 2;
         uint8_t red     : 2;
         uint8_t last    : 1; // msb
      };
      uint8_t color;
   };
   uint8_t size;  // unscaled size
   uint16_t angle;  // 0x7FF 10 bit angle
} vector_t; // 4 bytes

#define VFIELD_COLOR(row)   (((row)*4)+0)
#define VFIELD_SIZE(row)    (((row)*4)+1)
#define VFIELD_ANGLE_L(row) (((row)*4)+2)
#define VFIELD_ANGLE_H(row) (((row)*4)+3)
#define V_SIZE(x)            (x*4)

// L R R G G B B D
#define SEGA_VISIBLE       (0x01)
// it appears that SEGA_LAST cannot be used alone
// in the symbol table it must be combined with i.e. SEGA_VISIBLE|SEGA_LAST
// and in the vector table it must be combined with a non-zero color i.e. SEGA_COLOR_GRAY|SEGA_LAST
#define SEGA_LAST          (0x80)
#define SEGA_CLEAR         (0)
#define SEGA_COLOR_RED     (0x60|SEGA_VISIBLE)
#define SEGA_COLOR_GREEN   (0x18|SEGA_VISIBLE)
#define SEGA_COLOR_GREEN1  (0x10|SEGA_VISIBLE)
#define SEGA_COLOR_GREEN2  (0x08|SEGA_VISIBLE)
#define SEGA_COLOR_BLUE    (0x06|SEGA_VISIBLE)
#define SEGA_COLOR_YELLOW  (SEGA_COLOR_RED|SEGA_COLOR_GREEN)
#define SEGA_COLOR_CYAN    (SEGA_COLOR_GREEN|SEGA_COLOR_BLUE)
#define SEGA_COLOR_CYAN1   (0x14|SEGA_VISIBLE)
#define SEGA_COLOR_CYAN2   (0x0A|SEGA_VISIBLE)
#define SEGA_COLOR_MAGENTA (SEGA_COLOR_RED|SEGA_COLOR_BLUE)
#define SEGA_COLOR_ORANGE  (SEGA_COLOR_RED|SEGA_COLOR_GREEN1)
#define SEGA_COLOR_BRWHITE (0x7E|SEGA_VISIBLE)
#define SEGA_COLOR_WHITE   (0x54|SEGA_VISIBLE)
#define SEGA_COLOR_GRAY    (0x2A|SEGA_VISIBLE)
#define S_SIZE(x)          (x*10)

#define CENTER_X (1024)
#define CENTER_Y (1024)
#define MIN_X (1024-450) // 574
#define MAX_X (1024+450) // 1474
#define MIN_Y (1024-400) // 624
#define MAX_Y (1024+400) // 1424

#define GAME_TEXT_X     (MIN_X)
#define GAME_TEXT_Y     (MIN_Y+70)
#define GAME_TEXT_SIZE  0x80
#define SCORE_TEXT_X    (MAX_X-100)
#define SCORE_TEXT_Y    (MAX_Y-60)
#define SCORE_TEXT_SIZE 0x80
#define TIMER_TEXT_Y    (MAX_Y-80)
#define TIMER_TEXT_SIZE 0xF0

#define HITBOX_SZ       60
#define MAX_SYMBOLS     32

// sounds
#define COIN_DROP    0x24


typedef enum {
   game_state_boot,
   game_state_attract,
   game_state_play,
   game_state_game_over,
   game_state_game_over_pause,
   game_state_highscore,
   game_state_diagnostics_io_init,
   game_state_diagnostics_io,
   game_state_diagnostics_grid_init,
   game_state_diagnostics_grid,
} game_state_t;


typedef enum {
   IMPULSE = 0x04,
   IMPULSE_END = 0x05,
   WARP = 0x06,
   WARP_END = 0x07,
   RED_ALERT = 0x0C,
   RED_ALERT_END = 0x0D,
   PHASER = 0x08,
   PHOTON = 0x0A,
   TARGETING = 0x0E,
   DENY = 0x10,
   SHIELD_HIT = 0x12,
   ENTERPRISE_HIT = 0x14,
   ENTERPRISE_EXPLOSION = 0x16,
   KLINGON_EXPLOSION = 0x1A,
   DOCK = 0x1C,
   STARBASE_HIT = 0x1E,
   STARBASE_RED = 0x11,
   WARP_SUCK = 0x18,
   WARP_SUCK_END = 0x2F,
   SAUCER_EXIT = 0x19,
   SAUCER_EXIT_END = 0x2F,
   STARBASE_EXPLOSION = 0x22,
   SMALL_BONUS = 0x24,
   LARGE_BONUS = 0x25,
   STARBASE_INTRO = 0x26,
   KLINGON_INTRO = 0x27,
   ENTERPRISE_INTRO = 0x28,
   PLAYER_CHANGE = 0x29,
   KLINGON_FIRE = 0x2E,
   HIGH_SCORE_MUSIC = 0x2A,
   COIN_DROP_MUSIC = 0x2B,
   NOMAD_MOTION = 0x2C,
   NOMAD_MOTION_END = 0x21,
   NOMAD_STOPPED = 0x2D,
   NOMAD_STOPPED_END = 0x21,
} sound_t;

typedef enum {
   NO_PHRASE = 0x00,                // no phrase
   FERENGI_AH = 1,
   FERENGI_YES = 2,
   FERENGI_OOMOX = 3,
   POWER_FAILED = 4,
   POWER_RESTORED = 5,
   COUNT_2 = 6,
   COUNT_3 = 7,
   COUNT_4 = 8,
   COUNT_5 = 9,
} phrase_t;


////////////////////////////////////
// games
void shirtGame( void ); 
void crystalGame( void );
void ferengiGame( void ); 


////////////////////////////////////
// externs to main
void say(uint8_t i);
uint16_t spinner_vector_angle( bool reset );
void waitAnimate( uint16_t ticks );
void delay(uint16_t ms);

extern volatile uint16_t system_tick;
extern uint16_t score;

extern uint8_t* heap;

extern vector_t* vec_box;
extern vector_t* vec_string1;
extern vector_t* vec_string2;
extern vector_t* vec_string3;

extern symbol_t *const symbols;
extern symbol_t* sym_string1;
extern symbol_t* sym_string2;
extern symbol_t* sym_string3;


////////////////////////////////////
// math.h
uint8_t rand8(void);
uint16_t rand16(void);
inline uint16_t xy_multiply( uint8_t x, uint8_t y );
uint16_t div_16(uint16_t u, uint8_t v);
uint8_t divideBy10(uint8_t *value);
uint8_t divideBy100(uint8_t *value);
void vectorToXY( uint16_t sega_angle, uint16_t length, int16_t *x, int16_t *y );
uint16_t xyToVector(uint16_t x, uint16_t y);

#define divide1_5(x) (uint16_t)(((x)>>1)+((x)>>3)+((x)>>5))
// note: x < 8191
#define divide2_9(x) (uint16_t)(((uint16_t)(x)<<3)+((uint16_t)(x)<<1)+(x))>>5
#define divide5(x)   (uint16_t)(((x)>>3)+((x)>>4)+((x)>>6))
#define divide25(x)  (uint16_t)(((uint16_t)(x)<<3)+((uint16_t)(x)<<1))>>8

////////////////////////////////////
// font.h
uint16_t installFonts( uint16_t addr );
uint16_t fontAddress( char c );
void drawString( symbol_t *const sym, vector_t *const vec, uint16_t x, uint16_t y, uint8_t scale, uint8_t color, const char *const str);
uint16_t measureString(const char *str) __z88dk_fastcall;
void drawScore( uint16_t score, bool reset );
uint8_t drawCountdown( uint8_t initValue, bool speak );
void hex16( char *s, uint16_t value);
void dec2(char *s, uint8_t value);
void dec4(char *s, uint16_t value);
void colorize(vector_t *vec, uint16_t len, uint8_t color);

////////////////////////////////////
// vector.h
void animate(uint16_t system_tick, symbol_t* symbols, uint8_t symbol_count); 
void resetAnimate(void);
void setTrajectory( uint8_t sid, uint8_t velocity, uint16_t sega_angle );
void setSpeeds( uint8_t sid, int8_t x_speed, int8_t y_speed );
void setRotationSpeed( uint8_t sid, int8_t rotation_speed );
void setResizeSpeed( uint8_t sid, int8_t resize_speed );
void setStop( uint8_t sid );

void resetSymbol( symbol_t *const sym, uint16_t x, uint16_t y, uint16_t sega_angle, uint8_t scale );
void enableSymbol( symbol_t *const sym, uint16_t x, uint16_t y, uint16_t sega_angle, uint8_t scale );
void drawSymbol( symbol_t *const sym, vector_t *const vec, uint16_t x, uint16_t y, uint16_t sega_angle, uint8_t scale );

void initVector( vector_t *const vec, uint8_t size, uint16_t angle, uint8_t color );
void initSymbol( symbol_t *const sym, vector_t *const vec );
bool checkColission( symbol_t *const s0, symbol_t *const s1 );


////////////////////////////////////
// diagnostic.h
void beginDiagnosticsIO( void );
void drawDiagnosticsIO( void );
void beginDiagnosticsGrid( void );


#ifdef MAME_BUILD

// mame/sega/segag80v.cpp
// void segag80v_state::init_startrek()
//     ...
//     m_decrypt = segag80_security(0); // security free

// mame/src/devices/cpu/z80/z80.cpp
// void z80_device::data_write(u16 addr, u8 value)
// {
//    if ( addr == 0xF666 ) {
//       static u8 packet[5] = {0,};
//       packet[0] = packet[1];
//       packet[1] = packet[2];
//       packet[2] = packet[3];
//       packet[3] = packet[4];
//       packet[4] = value;
//       if ( packet[0] == 0xbe && packet[1] == 0xef ) {
//            char c = packet[2];
//             uint16_t v = (packet[3] << 8) + packet[4];
//             printf( "debug(%c): %04x(%d)\n", c, v, v );
//       }
//       return;
//    }
//    assert( SP > (0xC800+(1*1024)) );
//    assert( !(addr == 0x0000) );
//    assert( !(addr > 0xbfff && addr < 0xc800) );
//    assert( !(addr > 0xefff) );
//    m_data.write_interruptible(translate_memory_address((u32)addr), value);
// }

   #define writeDebug( c,v ) \
      do { \
        uint8_t *mame = (uint8_t*)(0xF666); \
        *mame = 0xBE; \
        *mame = 0xEF; \
        *mame = c; \
        *mame = MSB(v); \
        *mame = LSB(v); \
      } while(0)

   #define kill(x) \
      do { \
        writeDebug('!',x); \
        uint8_t *halt = (uint8_t*)(0x0000); \
        *halt = x; \
      } while(0)

#else 

   #define writeDebug(c,v) do {} while(0)
   #define kill(x) do {} while(0)

#endif

static inline void* _alloc_or_kill( void* p, uint16_t code ) { if (!p) kill(code); return p; }

#endif //_SEGA_H_
