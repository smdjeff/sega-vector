////////////////////////////////////
#include "sega.h"

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)


#define assert(x) \
     while ( !(x) ) { \
          writeDebug( '!', __LINE__ ); \
          uint8_t *halt = (uint8_t*)(0x0000); \
          *halt = 1;\
     };

#define assert2(x)  \
     if ( !(x) ) { \
          const char *ass = __FILE__ ":" TO_STRING(__LINE__); \
          drawString( (uint8_t*)S_ADDR(1), CENTER_X, CENTER_Y, 0x40, SEGA_COLOR_YELLOW, ass ); \
     }

 
 static void writeDebug( char c, uint16_t v ) {
     uint8_t *mame = (uint8_t*)(0xd000);
     *mame = 0xBE;
     *mame = 0xEF;
     *mame = c;
     *mame = MSB(v);
     *mame = LSB(v);
}


static void drawPort( void ) {
   uint8_t f8 = PORT_370;
   static uint8_t s[] = "12345678";
   s[0] = (f8 & BIT(0)) ? '1' : '0';
   s[1] = (f8 & BIT(1)) ? '1' : '0';
   s[2] = (f8 & BIT(2)) ? '1' : '0';
   s[3] = (f8 & BIT(3)) ? '1' : '0';
   s[4] = (f8 & BIT(4)) ? '1' : '0';
   s[5] = (f8 & BIT(5)) ? '1' : '0';
   s[6] = (f8 & BIT(6)) ? '1' : '0';
   s[7] = (f8 & BIT(7)) ? '1' : '0';
   drawString( (uint8_t*)S_ADDR(S_DIG1), CENTER_X-165, MIN_Y+40, 0x80, SEGA_COLOR_GREEN, s );
}


 static uint8_t sound_wait(void) {
   uint16_t timeout = 3000/50;
   while ( timeout-- ) {
      if ( (SOUND_COMMAND & 0xFE) == 0x80 ) {
         // 8035 ready for next command
         //delay( 250 ); // wait for sound to finish
         return 0;
      }
      delay( 50 );
   }
   return 1;
}


void sound_test(void) {
   const uint8_t a[] = {
      COIN_DROP_MUSIC,
      HIGH_SCORE_MUSIC,
      IMPULSE,
      IMPULSE_END,
      WARP,
      WARP_END,
      RED_ALERT,
      RED_ALERT_END,
      PHASER,
      PHOTON,
      TARGETING,
      DENY,
      SHIELD_HIT,
      ENTERPRISE_HIT,
      ENTERPRISE_EXPLOSION,
      KLINGON_EXPLOSION,
      DOCK,
      STARBASE_HIT,
      STARBASE_RED,
      WARP_SUCK,
      WARP_SUCK_END,
      SAUCER_EXIT,
      SAUCER_EXIT_END,
      STARBASE_EXPLOSION,
      SMALL_BONUS,
      LARGE_BONUS,
      STARBASE_INTRO,
      KLINGON_INTRO,
      ENTERPRISE_INTRO,
      PLAYER_CHANGE,
      KLINGON_FIRE,
      NOMAD_MOTION,
      NOMAD_MOTION_END,
      NOMAD_STOPPED,
      NOMAD_STOPPED_END,
   };
   for (uint8_t i=0; i<sizeof(a); i++) {
      uint8_t s = a[i];

      // for MUSIC we a different 2k loaded into the back half of the 8035
      if (s==COIN_DROP_MUSIC) {
         SOUND_COMMAND = 0xFF; // assert RAM LOAD latch
         memcpy( (uint8_t*)USB_RAM+USB_ROM_SZ_B, (uint8_t*)USB_ROM_B, USB_ROM_SZ_B );
         SOUND_COMMAND = 0x7F; 
      }
      // reload normal effects into the back half of the 8035
      if (s==IMPULSE) {
         SOUND_COMMAND = 0xFF; // assert RAM LOAD latch
         memcpy( (uint8_t*)USB_RAM+USB_ROM_SZ_B, (uint8_t*)USB_ROM_A+USB_ROM_SZ_B, USB_ROM_SZ_B );
         SOUND_COMMAND = 0x7F; 
      }

      sound_wait();
      SOUND_COMMAND = s;
      switch (s) {
         case ENTERPRISE_INTRO:
         case PLAYER_CHANGE:
            delay(3000);
            break;
         case COIN_DROP_MUSIC: 
            delay(5000);
            break;
         case HIGH_SCORE_MUSIC:
            delay(13000);
            break;
         case IMPULSE_END:
         case WARP_END:
         case RED_ALERT_END:
         case WARP_SUCK_END: //SAUCER_EXIT_END
         case NOMAD_MOTION_END: //NOMAD_STOPPED_END
            delay(500);
            break;
         default:
            delay(1000);
            break;
      }
   }
}


static void speech_0to9(uint8_t v) {
   if ( v <= 9 ) {
      say( ZERO + v);
   }
}

static void say8(uint8_t v) {
   uint8_t r = v; // remainder
   uint8_t d0 = divideBy100( &r ); 
   uint8_t d1 = divideBy10( &r );
   uint8_t d2 = r;
   if ( d0 ) {
      speech_0to9( d0 );
   }
   if ( d1 || d0 ) {
      speech_0to9( d1 );
   }
   speech_0to9( d2 );
}

