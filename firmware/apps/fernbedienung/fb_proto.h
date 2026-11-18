/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Daniel Pangerl */
/* Fernbedienung's protocol with Steckfeld SF1 (docs/fernbedienung/protocol.md) and what the screen is drawn from
 * (unity build, included once by app.c, after bw_ui.h). No float: the unity build has no FPU. */
#ifndef FB_PROTO_H
#define FB_PROTO_H

#define FB_PROTO_VERSION 1u

/* SysEx: F0 7D 46 42 <cmd> ... F7 (7D: non-commercial id; "FB") */
#define FB_SX_ID0 0x7Du
#define FB_SX_ID1 0x46u
#define FB_SX_ID2 0x42u

/* FM-1 -> app */
#define FB_CMD_HELLO 0x01u
/* app -> FM-1 */
#define FB_CMD_ACK 0x02u
#define FB_CMD_SCREEN 0x10u
#define FB_CMD_CONTROLS 0x11u
#define FB_CMD_PICTURE 0x20u
#define FB_CMD_LIVE 0x21u
#define FB_CMD_NOTICE 0x12u
#define FB_CMD_BAY 0x13u
#define FB_ROW_SEQ 0x04u                    /* LIST row flags: a patch that plays a sequence (not only MIDI in) */
#define FB_CMD_LIST 0x30u
#define FB_CMD_LIST_CLOSE 0x31u
#define FB_CMD_ASK_HELLO 0x7Fu

/* controls: CCs on the MIDI channel; encoders relative, two's complement (1 up, 127 down) */
#define FB_CC_KNOB 20u                      /* KNOB1..4: 20..23 */
#define FB_CC_SELECT 24u                    /* pages */
#define FB_CC_ALGO 25u                      /* the programmer's PREV / NEXT */
#define FB_CC_PRESETS 26u                   /* browse patches */
#define FB_CC_REC 27u                       /* 127 pressed, 0 released */
#define FB_CC_SAVE 28u
#define FB_CC_PLAY 29u                      /* 127 pressed, 0 released: the app runs or stops its transport */
#define FB_CC_CAT_TAP 40u                   /* + category: a tap, 127 */
#define FB_CC_CAT_TURN 50u                  /* + category: SELECT turned while its button is held, relative */

/* categories (the buttons) */
enum { FB_CAT_HOME, FB_CAT_SCL, FB_CAT_FX, FB_CAT_LFO, FB_CAT_ENV, FB_CAT_SEQ, FB_CAT_ARP, FB_CAT_GLO, FB_CAT_COUNT };

/* module kinds */
enum {
    FB_K_OUTPUT, FB_K_BRIDGE, FB_K_OSCILLATOR, FB_K_WAVETABLE, FB_K_STRING, FB_K_FM, FB_K_FILTER, FB_K_LFO,
    FB_K_ENVELOPE, FB_K_DELAY, FB_K_REVERB, FB_K_DISTORTION, FB_K_CHORUS, FB_K_EQ, FB_K_COMPRESSOR, FB_K_GRAIN,
    FB_K_CHORD, FB_K_ARP, FB_K_AUDIO_IN, FB_K_MIDI, FB_K_MACRO, FB_K_SEQUENCER,
    FB_K_DUMP, FB_K_COMBINE, FB_K_MAGE, FB_K_SMEAR, FB_K_COUNT
};

/* ---- the screen layout (pixels) */
#define FB_HEAD_Y 0u
#define FB_HEAD_H 22u
#define FB_PIC_X 0u
#define FB_PIC_Y 24u
#define FB_PIC_W 240u
#define FB_PIC_H 84u
#define FB_TILE_Y 110u                      /* 2 x 2 tiles of 118 x 52, 4 px apart */
#define FB_TILE_W 118u
#define FB_TILE_H 52u
#define FB_FOOT_Y 218u
#define FB_FOOT_H 22u

/* ---- the theme: Steckfeld's display (light: blue phosphor, dark: red); RGB565 */
typedef struct {
    uint16_t bg_top, bg_bottom;             /* the display's gradient */
    uint16_t ink, ink_dim, grid;            /* full ink, 40 %, the raster */
    uint16_t route_bg, route_ink;           /* the Route screen's own look */
} fb_theme_t;

/* ---- a module's picture, as the app sent it */
#define FB_PIC_VALUES 32u
#define FB_PIC_ARRAY 512u
#define FB_LIVE_VALUES 16u
#define FB_V_MAX 16383u                     /* 14-bit values */

typedef struct {
    uint8_t kind;                           /* FB_K_* */
    uint8_t n;                              /* values in v */
    uint16_t v[FB_PIC_VALUES];              /* 0..16383 each, the kind's layout (docs/fernbedienung/pictures-*.md) */
    uint16_t arr_n;                         /* bytes in arr */
    uint8_t arr[FB_PIC_ARRAY];              /* 0..127 each, the kind's layout */
    uint8_t live_n;
    uint16_t live[FB_LIVE_VALUES];          /* what moves (playhead, peaks, modulated values), the kind's layout */
    uint32_t serial;                        /* bumped by every PICTURE or LIVE: the drawer redraws on a change */
} fb_pic_t;

/* A picture drawer: draws pic into the region FB_PIC_X, FB_PIC_Y, FB_PIC_W x FB_PIC_H (a canvas of that size, then
   cv_blit), 1 if it knows pic->kind. The Route screen is the core's own. */
typedef int (*fb_pic_draw_fn)(const fb_pic_t* pic, const fb_theme_t* th);

/* the core's (fb_screen.h): cv_begin of the picture region, filled with the display's gradient and raster; a drawer
   may start from it, draw, then cv_blit(FB_PIC_X, FB_PIC_Y) */
static void fb_pic_begin(const fb_theme_t* th);

#endif
