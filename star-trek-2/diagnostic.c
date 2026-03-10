////////////////////////////////////
#include "sega.h"

void beginDiagnosticsIO( void ) {
   resetVectors();
   symbols[S_STRING].flags = SEGA_VISIBLE|SEGA_LAST;
   enableSymbol( S_SCORE0, CENTER_X-70, CENTER_Y-40, SEGA_ANGLE(0), 0xA0 );
   enableSymbol( S_SCORE1, CENTER_X-10, CENTER_Y-40, SEGA_ANGLE(0), 0xA0 );
   enableSymbol( S_SCORE2, CENTER_X+50, CENTER_Y-40, SEGA_ANGLE(0), 0xA0 );
}

void drawDiagnosticsIO( void ) {
   uint16_t angle = spinner_vector_angle(false);
   const char *hex = "0123456789abcdef";
   uint8_t d0 = hex[ (angle >> 8) & 0x0F ];
   uint8_t d1 = hex[ (angle >> 4) & 0x0F ];
   uint8_t d2 = hex[ (angle >> 0) & 0x0F ];
   symbols[ S_SCORE0 ].vector_addr = fontAddress( d0 );
   symbols[ S_SCORE1 ].vector_addr = fontAddress( d1 );
   symbols[ S_SCORE2 ].vector_addr = fontAddress( d2 );

   static uint8_t last_f8 = 0xFF;
   uint8_t f8 = PORT_370;
   if ( f8 != last_f8 ) {
      last_f8 = f8;
      static uint8_t s[8];
      s[0] = (f8 & BIT(0)) ? '1' : '0';
      s[1] = (f8 & BIT(1)) ? '1' : '0';
      s[2] = (f8 & BIT(2)) ? '1' : '0';
      s[3] = (f8 & BIT(3)) ? '1' : '0';
      s[4] = (f8 & BIT(4)) ? '1' : '0';
      s[5] = (f8 & BIT(5)) ? '1' : '0';
      s[6] = (f8 & BIT(6)) ? '1' : '0';
      s[7] = (f8 & BIT(7)) ? '1' : '0';
      drawString( &symbols[S_STRING], CENTER_X-100, CENTER_Y+40, 0x80, SEGA_COLOR_WHITE, s, 8 );
   }
}

void beginDiagnosticsGrid( void ) {
   // MAX_X-MIN_X = 900
   writeVec( &vectors[0], 240, SEGA_COLOR_RED, 0 );
   writeVec( &vectors[1], 240, SEGA_COLOR_BLUE, 0 );
   writeVec( &vectors[2], 240, SEGA_COLOR_GREEN, 0 );
   writeVec( &vectors[3], 180, SEGA_COLOR_WHITE, 1 );

   // MAX_Y-MIN_Y = 800
   writeVec( &vectors[4], 255, SEGA_COLOR_YELLOW, 0 );
   writeVec( &vectors[5],   9,  SEGA_COLOR_YELLOW, 0 );
   writeVec( &vectors[6], 255, SEGA_COLOR_CYAN, 0 );
   writeVec( &vectors[7],   9,  SEGA_COLOR_CYAN, 0 );
   writeVec( &vectors[8], 255, SEGA_COLOR_MAGENTA, 0 );
   writeVec( &vectors[9],  10,  SEGA_COLOR_MAGENTA, 1 );

   symbol_t * sym = symbols;
   for (uint16_t y=MIN_Y; y<MAX_Y+1; y+=(MAX_Y-MIN_Y)/12) {
      sym->visible = true;
      sym->last = false;
      sym->x = MIN_X;
      sym->y = y;
      sym->vector_addr = (uint16_t)&vectors[0];
      sym->rotation = SEGA_ANGLE(90);
      sym->scale = 0x80;
      sym++;
   }

   for (uint16_t x=MIN_X; x<MAX_X+1; x+=(MAX_X-MIN_X)/15) {
      sym->visible = true;
      sym->last = false;
      sym->x = x;
      sym->y = MIN_Y;
      sym->vector_addr = (uint16_t)&vectors[4];
      sym->rotation = SEGA_ANGLE(0);
      sym->scale = 0x80;
      sym++;
   }

   (sym-1)->last = true;
}


