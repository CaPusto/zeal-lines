;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler 
; Version 4.4.0 #14620 (Linux)
;--------------------------------------------------------
	.module CMakeCCompilerId
	.optsdcc -mz80
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _info_language_extensions_default
	.globl _info_language_standard_default
	.globl _info_arch
	.globl _info_platform
	.globl _info_compiler
	.globl _info_version
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
_info_compiler::
	.ds 2
_info_platform::
	.ds 2
_info_arch::
	.ds 2
_info_language_standard_default::
	.ds 2
_info_language_extensions_default::
	.ds 2
;--------------------------------------------------------
; absolute external ram data
;--------------------------------------------------------
	.area _DABS (ABS)
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area _HOME
	.area _GSINIT
	.area _GSFINAL
	.area _GSINIT
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area _HOME
	.area _HOME
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area _TEXT
;CMakeCCompilerId.c:907: int main(int argc, char* argv[])
;	---------------------------------
; Function main
; ---------------------------------
_main::
	push	ix
	ld	ix,#0
	add	ix,sp
	push	af
	push	af
	ex	de, hl
;CMakeCCompilerId.c:911: require += info_compiler[argc];
	ld	hl, (_info_compiler)
	add	hl, de
	ld	a, (hl)
	ld	-4 (ix), a
	ld	-3 (ix), #0x00
;CMakeCCompilerId.c:912: require += info_platform[argc];
	ld	hl, (_info_platform)
	add	hl, de
	ld	a, (hl)
	ld	c, #0x00
	add	a, -4 (ix)
	ld	-2 (ix), a
	ld	a, c
	adc	a, -3 (ix)
	ld	-1 (ix), a
;CMakeCCompilerId.c:913: require += info_arch[argc];
	ld	hl, (_info_arch)
	add	hl, de
	ld	c, (hl)
	ld	b, #0x00
	ld	a, -2 (ix)
	add	a, c
	ld	c, a
	ld	a, -1 (ix)
	adc	a, b
	ld	b, a
;CMakeCCompilerId.c:915: require += info_version[argc];
	ld	hl, #_info_version+0
	add	hl, de
	ld	l, (hl)
;	spillPairReg hl
	ld	h, #0x00
;	spillPairReg hl
;	spillPairReg hl
	add	hl, bc
	push	hl
	pop	iy
;CMakeCCompilerId.c:929: require += info_language_standard_default[argc];
	ld	hl, (_info_language_standard_default)
	add	hl, de
	ld	c, (hl)
	ld	b, #0x00
	add	iy, bc
;CMakeCCompilerId.c:930: require += info_language_extensions_default[argc];
	ld	hl, (_info_language_extensions_default)
	add	hl, de
	ld	c, (hl)
	ld	b, #0x00
	push	iy
	pop	hl
	add	hl, bc
;CMakeCCompilerId.c:932: return require;
	ex	de, hl
;CMakeCCompilerId.c:933: }
	ld	sp, ix
	pop	ix
	ret
_info_version:
	.db #0x49	; 73	'I'
	.db #0x4e	; 78	'N'
	.db #0x46	; 70	'F'
	.db #0x4f	; 79	'O'
	.db #0x3a	; 58
	.db #0x63	; 99	'c'
	.db #0x6f	; 111	'o'
	.db #0x6d	; 109	'm'
	.db #0x70	; 112	'p'
	.db #0x69	; 105	'i'
	.db #0x6c	; 108	'l'
	.db #0x65	; 101	'e'
	.db #0x72	; 114	'r'
	.db #0x5f	; 95
	.db #0x76	; 118	'v'
	.db #0x65	; 101	'e'
	.db #0x72	; 114	'r'
	.db #0x73	; 115	's'
	.db #0x69	; 105	'i'
	.db #0x6f	; 111	'o'
	.db #0x6e	; 110	'n'
	.db #0x5b	; 91
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x34	; 52	'4'
	.db #0x2e	; 46
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x34	; 52	'4'
	.db #0x2e	; 46
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x30	; 48	'0'
	.db #0x5d	; 93
	.db #0x00	; 0
	.area _TEXT
___str_0:
	.ascii "INFO:compiler[SDCC]"
	.db 0x00
___str_1:
	.ascii "INFO:platform[]"
	.db 0x00
___str_2:
	.ascii "INFO:arch[]"
	.db 0x00
___str_3:
	.ascii "INFO:standard_default[11]"
	.db 0x00
___str_4:
	.ascii "INFO:extensions_default[OFF]"
	.db 0x00
	.area _INITIALIZER
__xinit__info_compiler:
	.dw ___str_0
__xinit__info_platform:
	.dw ___str_1
__xinit__info_arch:
	.dw ___str_2
__xinit__info_language_standard_default:
	.dw ___str_3
__xinit__info_language_extensions_default:
	.dw ___str_4
	.area _CABS (ABS)
