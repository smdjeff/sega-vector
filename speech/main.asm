;==============================================================================
; SP0250 Raw LPC Frame Streamer
; Target CPU : Intel 8035 (MCS-48 family)
; Assembler  : asm48  https://github.com/daveho/asm48
;
; All addresses and hardware behaviour derived from the Ghidra decompilation
; of the original 8035 firmware (1670_speech-u7.c).
;
;------------------------------------------------------------------------------
; P2 USAGE  (from Ghidra)
;
;   P2 = 0x80  every init path (RESET, EXTIRQ, TIMIRQ, FEED_SP0250_LOOP).
;              WRITE_PKT_SP0250 never changes P2 -- inherits 0x80 from caller.
;              Both ROM reads (MOVX A,@R0) and SP0250 writes (MOVX @R0,A)
;              use P2=0x80.  Hardware tells them apart via RD/WR strobes.
;
;   P2 = 0x00  only inside RESPOND_7F (Z80 bus communication).
;
;------------------------------------------------------------------------------
; COMMAND PROTOCOL
;
;   0x00        → silence SP0250, return to idle.
;   anything else → play clip at index (command & 0x1F).
;
;------------------------------------------------------------------------------
; ROM INDEX TABLE  (external ROM, P2=0x80, base offset INDEX_TBL=0x00)
;
;   One 2-byte entry per clip:  lo_byte, hi_byte of LPC data start address.
;   hi_byte is OR'd with P2_SPEECH when loading into P2.
;
;   Example "20 00 42 00":
;     ROM[0x00]=0x20, ROM[0x01]=0x00  → clip 0 starts at P2=0x80, R0=0x20
;     ROM[0x02]=0x42, ROM[0x03]=0x00  → clip 1 starts at P2=0x80, R0=0x42
;     ROM[0x20]=...                   → clip 0 LPC frames
;     ROM[0x42]=...                   → clip 1 LPC frames
;
;   Lookup: clip index = command & 0x1F.  Table offset = index * 2.
;   MCS-48 has no MUL; use CLR C / RLC A for *2.
;
;------------------------------------------------------------------------------
; SP0250 REGISTER WRITES  (from WRITE_PKT_SP0250 in Ghidra)
;
;   15 bytes written sequentially from FRAME_BUF, descending from 0x2b.
;   byte[0] → 0x2b, byte[1] → 0x2a, ... byte[14] → 0x1d.
;   All with P2=0x80.  Always 15 bytes to SP0250 regardless of LPC_ORDER.
;   When LPC_ORDER<12, ROM frames are shorter; trailing buffer slots are
;   zero-padded by LOAD_FRAME so the SP0250 sees zeros for unused sections.
;
;------------------------------------------------------------------------------
; SILENCE FRAME VALUES  (from speech_01_lpc.txt silence frames)
;
;   All bytes 0x00 except:
;     byte[5] = 0xFF   PITCH  (PITCH_ init'd to 0xFF in every Ghidra reset path)
;     byte[8] = 0x41   REPEAT|VOICED  (PITCH=0x40 → (0x40 & 0x7F)+1 = 0x41)
;
;==============================================================================

;---------- BUILD FLAG: LPC ORDER ---------------------------------------------
; Set LPC_ORDER to 8, 10, or 12.  Change all three values together:
;
;   Order 12 (default):  ROM_FRAME_SZ=15, PAD_BYTES=0
;   Order 10:            ROM_FRAME_SZ=13, PAD_BYTES=2
;   Order  8:            ROM_FRAME_SZ=11, PAD_BYTES=4
;
.equ LPC_ORDER,       10       ; LPC filter order: 8, 10, or 12
.equ ROM_FRAME_SZ,    13       ; 3 + LPC_ORDER: bytes read from ROM per frame
.equ PAD_BYTES,        2       ; 15 - ROM_FRAME_SZ: zero-fill count
;-------------------------------------------------------------------------------

.equ P2_SPEECH,  0x80   ; P2 for speech ROM reads and SP0250 register writes.
.equ P2_Z80,     0x00   ; P2 for Z80 bus.
.equ INDEX_TBL,  0x00   ; ROM offset of clip index table (P2=0x80 page).
.equ SP0250_TOP, 0x2b   ; Highest SP0250 register address; write descends to 0x1d.
.equ FRAME_BUF,  0x10   ; Internal RAM: 15-byte frame buffer (0x10..0x1E).

; Register roles (bank 0):
;   R0 = external address low byte for MOVX
;   R1 = internal RAM pointer for @R1 indirect
;   R2 = byte counter
;   R3 = Z80 command byte
;   R4 = LPC ROM pointer low byte   (persists across frames)
;   R5 = LPC ROM pointer high byte  (persists across frames; OR'd with P2_SPEECH)

;==============================================================================
.org 0x00

RESET:
        DIS  I
        DIS  TCNTI

        MOV  A, #P2_SPEECH
        OUTL P2, A

        CALL SILENCE_FRAME
        CALL WRITE_FRAME

;==============================================================================
; CMD_WAIT -- idle: spin until Z80 sends a command.
;==============================================================================
CMD_WAIT:
        JNT0 CMD_WAIT

;==============================================================================
; CMD_HANDLER -- read command, silence or look up clip address and play.
;==============================================================================
CMD_HANDLER:
        MOV  A, #P2_Z80
        OUTL P2, A

        IN   A, P1
        MOV  R3, A

        MOV  A, #0x7f       ; acknowledge to Z80
        OUTL P1, A
        MOV  A, #0xff       ; release bus
        OUTL P1, A

        MOV  A, #P2_SPEECH
        OUTL P2, A

        ; 0x00 → silence + stop.
        MOV  A, R3
        JNZ  CMD_PLAY
        CALL SILENCE_FRAME
        CALL WRITE_FRAME
        JMP  CMD_WAIT

        ; Anything else → play clip at index (command & 0x1F).
CMD_PLAY:
        MOV  A, R3
        ANL  A, #0x1f       ; clip index
        CLR  C
        RLC  A              ; index * 2 (no MUL in MCS-48)
        ADD  A, #INDEX_TBL
        MOV  R0, A

        MOVX A, @R0         ; lo byte of clip address
        MOV  R4, A
        INC  R0
        MOVX A, @R0         ; hi byte of clip address
        MOV  R5, A

;==============================================================================
; MAIN_LOOP -- stream frames until end-of-data or new command.
;==============================================================================
MAIN_LOOP:
        JT0  CMD_HANDLER

WAIT_DRQ:
        JT0  CMD_HANDLER
        JNT1 WAIT_DRQ

        CALL LOAD_FRAME

        ; End-of-stream: repeat field byte[8] bits[5:0] == 0.
        MOV  R1, #FRAME_BUF + 8
        MOV  A, @R1
        ANL  A, #0x3f
        JZ   END_OF_DATA

        CALL WRITE_FRAME
        JMP  MAIN_LOOP

END_OF_DATA:
        CALL SILENCE_FRAME
        CALL WRITE_FRAME
        JMP  CMD_WAIT

;==============================================================================
; LOAD_FRAME
;   Read ROM_FRAME_SZ bytes from external ROM into FRAME_BUF, then zero-pad
;   any remaining bytes so SP0250 always sees a full 15-byte frame.
;   P2 = P2_SPEECH | R5,  address low = R4.
;   R4/R5 advanced by ROM_FRAME_SZ on return.
;   Trashes: A, R0, R1, R2
;==============================================================================
LOAD_FRAME:
        MOV  A, R5
        ORL  A, #P2_SPEECH
        OUTL P2, A

        MOV  A, R4
        MOV  R0, A

        MOV  R1, #FRAME_BUF
        MOV  R2, #ROM_FRAME_SZ

LF_LOOP:
        MOVX A, @R0
        MOV  @R1, A
        INC  R1
        INC  R0
        MOV  A, R0
        JNZ  LF_NEXT
        INC  R5
        MOV  A, R5
        ORL  A, #P2_SPEECH
        OUTL P2, A
LF_NEXT:
        DJNZ R2, LF_LOOP

        ; Zero-pad remaining bytes (PAD_BYTES) for SP0250.
        ; When LPC_ORDER=12, PAD_BYTES=0 and the pad loop is skipped.
        MOV  R2, #PAD_BYTES
        MOV  A, R2
        JZ   LF_DONE
        CLR  A
LF_PAD:
        MOV  @R1, A
        INC  R1
        DJNZ R2, LF_PAD

LF_DONE:
        MOV  A, R0
        MOV  R4, A
        RET

;==============================================================================
; WRITE_FRAME
;   Write all 15 bytes from FRAME_BUF to SP0250 registers, descending
;   from SP0250_TOP (0x2b) to 0x1d.  byte[0] → 0x2b, byte[14] → 0x1d.
;   Always writes 15 bytes regardless of LPC_ORDER.
;   Trashes: A, R0, R1, R2
;==============================================================================
WRITE_FRAME:
        MOV  A, #P2_SPEECH
        OUTL P2, A

        MOV  R0, #SP0250_TOP
        MOV  R1, #FRAME_BUF
        MOV  R2, #15

WF_LOOP:
        MOV  A, @R1
        MOVX @R0, A
        INC  R1
        MOV  A, R0
        DEC  A
        MOV  R0, A
        DJNZ R2, WF_LOOP
        RET

;==============================================================================
; SILENCE_FRAME
;   Fill FRAME_BUF with a valid silent LPC frame (always 15 bytes for SP0250).
;   Values from speech_01_lpc.txt silence frames:
;     byte[5] = 0xFF  (PITCH_ init to 0xFF in every Ghidra reset path)
;     byte[8] = 0x41  (PITCH=0x40 → (0x40 & 0x7F)+1 = 0x41)
;   All other bytes 0x00.
;   Trashes: A, R1, R2
;==============================================================================
SILENCE_FRAME:
        MOV  R1, #FRAME_BUF
        MOV  R2, #15
        CLR  A
SF_LOOP:
        MOV  @R1, A
        INC  R1
        DJNZ R2, SF_LOOP

        MOV  R1, #FRAME_BUF + 5
        MOV  A, #0xff
        MOV  @R1, A

        MOV  R1, #FRAME_BUF + 8
        MOV  A, #0x41
        MOV  @R1, A
        RET
