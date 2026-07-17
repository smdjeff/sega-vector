
// data section copied from ROM into RAM on program start
#pragma output CRT_MODEL=1

// start of RAM on Sega G80
#pragma output CRT_ORG_DATA=0xC800
#pragma output REGISTER_SP=0xCFFF

#pragma output CRT_ENABLE_RST=0x80
#pragma output CRT_ENABLE_NMI=1
#pragma output CRT_ENABLE_EIDI=0x01
#pragma output CRT_INTERRUPT_MODE=1
#pragma output CLIB_MALLOC_HEAP_SIZE=0
#pragma output CLIB_STDIO_HEAP_SIZE=0

#ifdef ENABLE_BOOTROM

  #pragma output CRT_ORG_CODE = 0x0800 // when using boot rom

//  Simply setting CRT_ORG_VECTOR_TABLE will not work, since it's
//  ignored when CRT_ORG_CODE is non zero. Workaround is to patch 
//  z88dk/libsrc/_DEVELOPMENT/target/z80/startup/z80_crt_0.asm.m4 as follows:
//  ;IF (ASMPC = 0) && (__crt_org_code = 0)
//    include "../crt_page_zero_z80.inc"
//  ;ENDIF

#else

  #pragma output CRT_ORG_CODE = 0x0000

#endif

