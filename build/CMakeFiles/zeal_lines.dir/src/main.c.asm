;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler 
; Version 4.4.0 #14620 (Linux)
;--------------------------------------------------------
	.module main
	.optsdcc -mz80
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _gfx_tilemap_load
	.globl _gfx_tileset_add_color_tile
	.globl _gfx_palette_load
	.globl _gfx_enable_screen
	.globl _gfx_initialize
	.globl _input_read
	.globl _input_init
	.globl _puts
	.globl _printf
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
_zvb_config_ver_rev	=	0x0080
_zvb_config_ver_min	=	0x0081
_zvb_config_ver_maj	=	0x0082
_zvb_config_dev_idx	=	0x008e
_zvb_config_phys_addr	=	0x008f
_zvb_ctrl_vpos_low	=	0x0090
_zvb_ctrl_vpos_high	=	0x0091
_zvb_ctrl_hpos_low	=	0x0092
_zvb_ctrl_hpos_high	=	0x0093
_zvb_ctrl_l0_scr_y_low	=	0x0094
_zvb_ctrl_l0_scr_y_high	=	0x0095
_zvb_ctrl_l0_scr_x_low	=	0x0096
_zvb_ctrl_l0_scr_x_high	=	0x0097
_zvb_ctrl_l1_scr_y_low	=	0x0098
_zvb_ctrl_l1_scr_y_high	=	0x0099
_zvb_ctrl_l1_scr_x_low	=	0x009a
_zvb_ctrl_l1_scr_x_high	=	0x009b
_zvb_ctrl_video_mode	=	0x009c
_zvb_ctrl_status	=	0x009d
_zvb_peri_text_print_char	=	0x00a0
_zvb_peri_text_curs_y	=	0x00a1
_zvb_peri_text_curs_x	=	0x00a2
_zvb_peri_text_scroll_y	=	0x00a3
_zvb_peri_text_scroll_x	=	0x00a4
_zvb_peri_text_color	=	0x00a5
_zvb_peri_text_curs_time	=	0x00a6
_zvb_peri_text_curs_char	=	0x00a7
_zvb_peri_text_curs_color	=	0x00a8
_zvb_peri_text_ctrl	=	0x00a9
_zvb_peri_spi_ctrl	=	0x00a1
_zvb_peri_spi_clk_div	=	0x00a2
_zvb_peri_spi_ram_len	=	0x00a3
_zvb_peri_spi_fifo	=	0x00a7
_zvb_peri_spi_array_0	=	0x00a8
_zvb_peri_spi_array_1	=	0x00a9
_zvb_peri_spi_array_2	=	0x00aa
_zvb_peri_spi_array_3	=	0x00ab
_zvb_peri_spi_array_4	=	0x00ac
_zvb_peri_spi_array_5	=	0x00ad
_zvb_peri_spi_array_6	=	0x00ae
_zvb_peri_spi_array_7	=	0x00af
_zvb_peri_crc_ctrl	=	0x00a0
_zvb_peri_crc_data_in	=	0x00a1
_zvb_peri_crc_byte0	=	0x00a4
_zvb_peri_crc_byte1	=	0x00a5
_zvb_peri_crc_byte2	=	0x00a6
_zvb_peri_crc_byte3	=	0x00a7
_zvb_peri_sound_freq_low	=	0x00a0
_zvb_peri_sound_freq_high	=	0x00a1
_zvb_peri_sound_wave	=	0x00a2
_zvb_peri_sound_volume	=	0x00a3
_zvb_peri_sound_sample_fifo	=	0x00a0
_zvb_peri_sound_sample_div	=	0x00a1
_zvb_peri_sound_sample_conf	=	0x00a2
_zvb_peri_sound_left_channel	=	0x00ab
_zvb_peri_sound_right_channel	=	0x00ac
_zvb_peri_sound_hold	=	0x00ad
_zvb_peri_sound_master_vol	=	0x00ae
_zvb_peri_sound_select	=	0x00af
_zvb_peri_dma_ctrl	=	0x00a0
_zvb_peri_dma_addr0	=	0x00a1
_zvb_peri_dma_addr1	=	0x00a2
_zvb_peri_dma_addr2	=	0x00a3
_zvb_peri_dma_clk_div	=	0x00a9
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
_video:
	.ds 5
_main_tilemap_10001_221:
	.ds 40
_main_attributes_10001_221:
	.ds 40
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
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
;/src/src/main.c:31: int main(void)
;	---------------------------------
; Function main
; ---------------------------------
_main::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	hl, #-5
	add	hl, sp
	ld	sp, hl
;/src/src/main.c:34: zos_err_t err = input_init(1);
	ld	a, #0x01
	call	_input_init
	ld	-2 (ix), a
	ld	-1 (ix), a
;/src/src/main.c:35: if (err != ERR_SUCCESS) {
	ld	a, -2 (ix)
	or	a, a
	jr	Z, 00102$
;/src/src/main.c:36: printf("INPUT INIT ERROR: %d\n", err);
	ld	a, -1 (ix)
	ld	-2 (ix), a
	ld	-1 (ix), #0x00
	ld	l, -2 (ix)
;	spillPairReg hl
;	spillPairReg hl
	ld	h, #0x00
;	spillPairReg hl
;	spillPairReg hl
	push	hl
	ld	hl, #___str_0
	push	hl
	call	_printf
	pop	af
	pop	af
;/src/src/main.c:37: return 1;
	ld	de, #0x0001
	jp	00130$
00102$:
;/src/src/main.c:41: gfx_enable_screen(0);
	xor	a, a
	call	_gfx_enable_screen
;/src/src/main.c:46: &video) != GFX_SUCCESS) {
;/src/src/main.c:45: ZVB_CTRL_VID_MODE_GFX_640_4BIT,
	ld	de, #_video
	ld	a, #0x06
	call	_gfx_initialize
	ld	-1 (ix), a
	or	a, a
	jr	Z, 00104$
;/src/src/main.c:47: printf("VIDEO INIT ERROR\n");
	ld	hl, #___str_2
	call	_puts
;/src/src/main.c:48: return 1;
	ld	de, #0x0001
	jp	00130$
00104$:
;/src/src/main.c:54: (void*)test_palette,
;/src/src/main.c:53: &video,
	xor	a, a
	push	af
	inc	sp
	ld	hl, #0x0020
	push	hl
	ld	de, #_test_palette
	ld	hl, #_video
	call	_gfx_palette_load
	or	a, a
	jr	Z, 00106$
;/src/src/main.c:57: printf("PALETTE ERROR\n");
	ld	hl, #___str_4
	call	_puts
;/src/src/main.c:58: return 1;
	ld	de, #0x0001
	jp	00130$
00106$:
;/src/src/main.c:64: for (uint8_t i = 0; i < 16; i++) {
	ld	-5 (ix), #0x00
	ld	-1 (ix), #0x00
00122$:
	ld	a, -1 (ix)
	sub	a, #0x10
	jr	NC, 00139$
;/src/src/main.c:66: &video, i, (uint8_t)(i * 0x11)) != GFX_SUCCESS) {
	ld	a, -1 (ix)
	ld	c, a
	add	a, a
	add	a, a
	add	a, a
	add	a, a
	add	a, c
	ld	-4 (ix), a
	ld	a, -1 (ix)
	ld	-3 (ix), a
	ld	-2 (ix), #0x00
	ld	a, -4 (ix)
	push	af
	inc	sp
	ld	e, -3 (ix)
	ld	d, -2 (ix)
	ld	hl, #_video
	call	_gfx_tileset_add_color_tile
	ld	-2 (ix), a
	or	a, a
	jr	Z, 00123$
;/src/src/main.c:67: printf("TILE ERROR: %u\n", i);
	ld	a, -5 (ix)
;	spillPairReg hl
;	spillPairReg hl
	ld	-2 (ix), a
	ld	-1 (ix), #0x00
	ld	l, a
	ld	h, #0x00
;	spillPairReg hl
;	spillPairReg hl
	push	hl
	ld	hl, #___str_5
	push	hl
	call	_printf
	pop	af
	pop	af
;/src/src/main.c:68: return 1;
	ld	de, #0x0001
	jp	00130$
00123$:
;/src/src/main.c:64: for (uint8_t i = 0; i < 16; i++) {
	inc	-1 (ix)
	ld	a, -1 (ix)
	ld	-5 (ix), a
	jr	00122$
;/src/src/main.c:77: for (uint8_t x = 0; x < 40; x++) {
00139$:
	ld	c, #0x00
00125$:
	ld	a, c
	sub	a, #0x28
	jr	NC, 00110$
;/src/src/main.c:78: tilemap[x] = x % 16;
	ld	hl, #_main_tilemap_10001_221
	ld	b, #0x00
	add	hl, bc
	ld	a, c
	and	a, #0x0f
	ld	(hl), a
;/src/src/main.c:79: attributes[x] = 0;
	ld	hl, #_main_attributes_10001_221
	ld	b, #0x00
	add	hl, bc
	ld	(hl), #0x00
;/src/src/main.c:77: for (uint8_t x = 0; x < 40; x++) {
	inc	c
	jr	00125$
00110$:
;/src/src/main.c:83: for (uint8_t y = 0; y < 30; y++) {
	ld	-1 (ix), #0x00
00128$:
	ld	a, -1 (ix)
	sub	a, #0x1e
	jr	NC, 00115$
;/src/src/main.c:85: &video, tilemap, sizeof(tilemap), 0, 0, y) != GFX_SUCCESS) {
	ld	h, -1 (ix)
	ld	l, #0x00
	push	hl
	xor	a, a
	push	af
	inc	sp
	ld	a, #0x28
	push	af
	inc	sp
	ld	de, #_main_tilemap_10001_221
	ld	hl, #_video
	call	_gfx_tilemap_load
	or	a, a
	jr	Z, 00112$
;/src/src/main.c:86: printf("TILEMAP ERROR\n");
	ld	hl, #___str_7
	call	_puts
;/src/src/main.c:87: return 1;
	ld	de, #0x0001
	jr	00130$
00112$:
;/src/src/main.c:92: &video, attributes, sizeof(attributes), 1, 0, y) != GFX_SUCCESS) {
	ld	a, -1 (ix)
	push	af
	inc	sp
	xor	a, a
	ld	h, a
	ld	l, #0x01
	push	hl
	ld	a, #0x28
	push	af
	inc	sp
	ld	de, #_main_attributes_10001_221
	ld	hl, #_video
	call	_gfx_tilemap_load
	or	a, a
	jr	Z, 00129$
;/src/src/main.c:93: printf("ATTRIBUTE ERROR\n");
	ld	hl, #___str_9
	call	_puts
;/src/src/main.c:94: return 1;
	ld	de, #0x0001
	jr	00130$
00129$:
;/src/src/main.c:83: for (uint8_t y = 0; y < 30; y++) {
	inc	-1 (ix)
	jr	00128$
00115$:
;/src/src/main.c:99: gfx_enable_screen(1);
	ld	a, #0x01
	call	_gfx_enable_screen
;/src/src/main.c:108: while (1) {
00119$:
;/src/src/main.c:109: uint16_t buttons = input_read();
	call	_input_read
;/src/src/main.c:111: if (buttons & BUTTON_SELECT) {
	bit	2, e
	jr	Z, 00119$
;/src/src/main.c:116: return 0;
	ld	de, #0x0000
00130$:
;/src/src/main.c:117: }
	ld	sp, ix
	pop	ix
	ret
_test_palette:
	.dw #0x0000
	.dw #0xffff
	.dw #0xf800
	.dw #0x07e0
	.dw #0x001f
	.dw #0xffe0
	.dw #0xfc00
	.dw #0x801f
	.dw #0x07ff
	.dw #0xf81f
	.dw #0x8410
	.dw #0x4208
	.dw #0x87f0
	.dw #0xfc10
	.dw #0x841f
	.dw #0xfff0
___str_0:
	.ascii "INPUT INIT ERROR: %d"
	.db 0x0a
	.db 0x00
___str_2:
	.ascii "VIDEO INIT ERROR"
	.db 0x00
___str_4:
	.ascii "PALETTE ERROR"
	.db 0x00
___str_5:
	.ascii "TILE ERROR: %u"
	.db 0x0a
	.db 0x00
___str_7:
	.ascii "TILEMAP ERROR"
	.db 0x00
___str_9:
	.ascii "ATTRIBUTE ERROR"
	.db 0x00
	.area _TEXT
	.area _INITIALIZER
	.area _CABS (ABS)
