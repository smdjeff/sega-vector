;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; sega-boot.asm
; Sega G80 Bootload ROM (allows program to exist only in ROM board)
; 1873.cpu-u25
; v1.0 Sept 17, 2024
; Jeff Mathews
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	JP  0800h  ; Reset Vector

;	ds	8h-$	; RST 1
;	JP  0808h

;	ds	10h-$	; RST 2
;	JP  0810h

;	ds	18h-$	; RST 3
;	JP  0818h

;	ds	20h-$   ; RST 4
;	JP  0820h

;	ds	28h-$   ; RST 5
;	JP  0828h

;	ds	30h-$	; RST 6
;	JP  0830h

	ds	38h-$   ; IM 1
	JP  0838h
;	EI
;	RETI

	ds	66h-$   ; NMI
	JP  0866h
;	RETN
	

