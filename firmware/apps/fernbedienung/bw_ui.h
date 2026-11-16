/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Daniel Pangerl */
/* Beiwagen's screen kit (unity build, included once by app.c). Flat tiles, large type (Inter), the OP-1's encoder
 * colours for the four knobs. A page is a header, a 2 x 2 grid of tiles (one per knob, KNOB1..4, or the page's own)
 * and a footer. Sections (HOME, GLO, SEQ...) hold pages; SELECT steps through them.
 *
 *   0..21     header: the page's title; the page's own corner (the FX chain) or the page dots
 *   24..215   tiles of 118 x 94, 4 px apart
 *   218..239  footer: the source (ALGORITHM), the status
 *
 * Redraws stay small: a page change draws every region over the last, top to bottom (no clear); a knob redraws its
 * tile's value and bar; a tile lights while its knob is turned and dims again a moment after. */
#ifndef BW_UI_H
#define BW_UI_H

#define UI_HEAD_H 22u
#define UI_GRID_Y 24u
#define UI_TILE_W 118u
#define UI_TILE_H 94u
#define UI_GAP 4u
#define UI_FOOT_Y 218u
#define UI_FOOT_H 22u
#define UI_LIT_MS 1200u                     /* a tile stays lit this long after its knob moved */
#define METER_DB_MIN (-600)                 /* meters' bottom, tenths of a dB */

#define C_BG RGB(0, 0, 0)
#define C_TILE RGB(22, 22, 28)              /* a tile at rest */
#define C_TILE_HI RGB(40, 40, 50)
#define C_TEXT RGB(236, 236, 240)
#define C_SUB RGB(140, 140, 152)
#define C_WARN RGB(255, 196, 40)

/* the OP-1's encoder colours, knob 1..4 */
static const uint16_t UI_KNOB_C[4] = { RGB(40, 120, 255), RGB(30, 200, 100), RGB(236, 236, 240), RGB(255, 100, 30) };

/* a parameter: an integer in [min, max], one knob detent = step; apply() hands a new value on (may be 0); tag is
   the owner's (an effect's index, a parameter's) */
typedef struct ui_param ui_param_t;
struct ui_param {
    const char* name;
    int32_t min, max, step;
    int32_t* val;
    void (*fmt)(char* b, int32_t v, const ui_param_t* p);   /* 0: the plain number */
    void (*apply)(int32_t v, const ui_param_t* p);
    int32_t tag;
};

typedef struct {
    const char* title;
    const ui_param_t* p[4];                 /* the knobs' parameters, 0: none */
    void (*tiles)(uint32_t ms, int full);   /* set: the page draws the grid itself (the tracks on HOME) */
    void (*corner)(uint32_t ms, int full);  /* set: the header's right side (the FX chain); else the page dots */
    void (*screen)(uint32_t ms, int full);  /* set: the page draws the whole screen (no header, tiles, footer) */
    uint32_t corner_w;                      /* the corner's width (the title is cut before it) */
} ui_page_t;

typedef struct {
    const char* name;
    uint32_t button;                        /* FW_BTN_*: pressing it opens the section */
    const ui_page_t* const* pages;
    uint32_t count;
} ui_section_t;

static const ui_page_t* ui_cur;            /* the page on the screen (a drawer may need to know which it is) */

/* 0..32768 (32768 = 0 dB) in tenths of a dB, from -960; within about 0.1 dB */
static int32_t db10(uint32_t v)
{
    uint32_t b = 0, f;
    int32_t l;
    if (!v)
        return -960;
    while (b < 31u && (v >> (b + 1u)))
        b++;
    f = b >= 16u ? (v >> (b - 16u)) & 0xFFFFu : (v << (16u - b)) & 0xFFFFu;   /* the mantissa past 1, Q16 */
    f += ((f * (65536u - f)) >> 16) * 22u >> 6;                                /* log2(1 + f), near enough */
    l = ((int32_t)b - 15) * 65536 + (int32_t)f;                                 /* log2(v / 32768), Q16 */
    l = (l * 602 >> 16) / 10;
    return l < -960 ? -960 : l;
}

static uint16_t meter_colour(int32_t db)
{
    return db > -30 ? RGB(255, 60, 40) : db > -120 ? RGB(250, 200, 40) : RGB(30, 204, 112);
}

/* a level 0..32768 as 0..n pixels, -60 dB .. 0 */
static int32_t ui_level(uint32_t peak, int32_t n)
{
    int32_t v = (db10(peak) - METER_DB_MIN) * n / -METER_DB_MIN;
    return v < 0 ? 0 : v > n ? n : v;
}

/* a horizontal level bar on the open canvas */
static void ui_meter(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t peak, uint16_t back)
{
    cv_rect(x, y, w, h, back);
    cv_rect(x, y, ui_level(peak, w), h, meter_colour(db10(peak)));
}

/* text anti-aliased onto bg (cv_text blends onto black only) */
static uint16_t ui_mix(uint16_t bg, uint16_t fg, uint32_t a)   /* a 0..15 */
{
    const uint32_t r = ((uint32_t)(bg >> 11) * (15u - a) + (uint32_t)(fg >> 11) * a) / 15u;
    const uint32_t g = ((uint32_t)((bg >> 5) & 63u) * (15u - a) + (uint32_t)((fg >> 5) & 63u) * a) / 15u;
    const uint32_t b = ((uint32_t)(bg & 31u) * (15u - a) + (uint32_t)(fg & 31u) * a) / 15u;
    return (uint16_t)(r << 11 | g << 5 | b);
}

static int32_t ui_text(int32_t x, int32_t y, const felucca_font_t* f, const char* s, uint16_t fg, uint16_t bg)
{
    uint16_t ramp[16];
    uint32_t a;
    for (a = 0; a < 16u; a++)
        ramp[a] = ui_mix(bg, fg, a);
    for (; *s; s++) {
        const uint32_t gi = glyph(f, (uint8_t)*s), w = f->bw[gi], bpr = ((w >> f->sh) + 1u) / 2u;
        const uint8_t* gd = f->data + f->off[gi];
        uint32_t gx, gy;
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                const uint32_t sx = gx >> f->sh;
                uint32_t v = gd[(gy >> f->sh) * bpr + sx / 2u];
                v = (sx & 1u) ? (v & 15u) : (v >> 4);
                if (v)
                    cv_pset(x - f->pad + (int32_t)gx, y + (int32_t)gy, ramp[v]);
            }
        x += f->adv[gi];
    }
    return x;
}

/* text cut to max_w pixels; returns the end x */
static int32_t ui_text_cut(int32_t x, int32_t y, const felucca_font_t* f, const char* s, int32_t max_w, uint16_t fg,
                           uint16_t bg)
{
    char b[48];
    uint32_t n = 0;
    int32_t w = 0;
    while (s[n] && n + 1u < sizeof b) {
        const int32_t a = f->adv[glyph(f, (uint8_t)s[n])];
        if (w + a > max_w)
            break;
        b[n] = s[n];
        w += a;
        n++;
    }
    b[n] = 0;
    return ui_text(x, y, f, b, fg, bg);
}

/* a label on a filled chip (REC, SAVE): returns its width */
static int32_t ui_chip(int32_t x, int32_t y, const char* s, uint16_t fill, uint16_t ink)
{
    const int32_t w = text_w(&FONT_T, s) + 8;
    cv_rect(x, y, w, (int32_t)FONT_T.h + 1, fill);
    ui_text(x + 4, y, &FONT_T, s, ink, fill);
    return w;
}

static void ui_cat(char* b, const char* s, unsigned max) { str_cpy(b + str_len(b), s, max - str_len(b)); }

static void ui_cat_int(char* b, int32_t v, unsigned max)
{
    char n[12];
    fmt_int(n, v);
    ui_cat(b, n, max);
}

/* ---- the frame */
static uint32_t ui_tile_x(uint32_t i) { return (i & 1u) * (UI_TILE_W + UI_GAP); }
static uint32_t ui_tile_y(uint32_t i) { return UI_GRID_Y + (i >> 1) * (UI_TILE_H + UI_GAP); }

static void ui_header(const ui_section_t* s, uint32_t page)
{
    const ui_page_t* pg = s->pages[page];
    const int32_t dots = !pg->corner && s->count > 1u ? 9 * (int32_t)s->count : 0;
    uint32_t i;
    cv_begin(240, UI_HEAD_H, C_BG);
    ui_text_cut(4, 0, &FONT_M, pg->title, (pg->corner ? 240 - (int32_t)pg->corner_w : 236 - dots) - 10, C_TEXT, C_BG);
    if (dots)                               /* where SELECT is: a dot per page */
        for (i = 0; i < s->count; i++)
            cv_rect(232 - 9 * (int32_t)(s->count - 1u - i), 8, 6, 6, i == page ? C_TEXT : C_TILE_HI);
    cv_blit(0, 0);
}

/* the strips between the regions and the grid's gaps (drawn once with a page) */
static void ui_gaps(void)
{
    lcd_fill(0, UI_HEAD_H, 240, UI_GRID_Y - UI_HEAD_H, C_BG);
    lcd_fill(UI_TILE_W, UI_GRID_Y, UI_GAP, 2u * UI_TILE_H + UI_GAP, C_BG);
    lcd_fill(0, UI_GRID_Y + UI_TILE_H, 240, UI_GAP, C_BG);
    lcd_fill(0, UI_GRID_Y + 2u * UI_TILE_H + UI_GAP, 240, UI_FOOT_Y - UI_GRID_Y - 2u * UI_TILE_H - UI_GAP, C_BG);
}

/* ---- a knob's tile: its colour along the top, the name, the value large, a bar; lit: filled with its colour */
#define UI_VAL_Y 34u                        /* the value's row inside a tile */
#define UI_BAR_Y (UI_TILE_H - 10u)

static void ui_value(char* b, const ui_param_t* p)
{
    if (p->fmt)
        p->fmt(b, *p->val, p);
    else
        fmt_int(b, *p->val);
}

static void ui_tile_bar(uint32_t i, const ui_param_t* p, uint16_t bg, uint16_t fg)
{
    const int32_t span = p->max - p->min, w = (int32_t)UI_TILE_W - 16;
    cv_begin(UI_TILE_W - 16u, 4, ui_mix(bg, fg, 4));
    cv_rect(0, 0, span > 0 ? (*p->val - p->min) * w / span : 0, 4, fg);
    cv_blit(ui_tile_x(i) + 8u, ui_tile_y(i) + UI_BAR_Y);
}

/* the value, in a strip the full width of the tile (a shorter value leaves nothing behind): the number large, its
   unit (what follows the first space) small beside it; what does not fit large is drawn medium */
static void ui_tile_value(uint32_t i, const ui_param_t* p, uint16_t bg, uint16_t fg)
{
    char b[20];
    char* unit = b;
    ui_value(b, p);
    while (*unit && *unit != ' ')
        unit++;
    if (*unit)
        *unit++ = 0;
    cv_begin(UI_TILE_W, FONT_V.h, bg);
    if (text_w(&FONT_V, b) + (*unit ? 4 + text_w(&FONT_M, unit) : 0) <= (int32_t)UI_TILE_W - 12) {
        const int32_t x = ui_text(8, 0, &FONT_V, b, fg, bg);
        if (*unit)
            ui_text(x + 4, (int32_t)FONT_V.h - (int32_t)FONT_M.h - 4, &FONT_M, unit, fg, bg);
    } else {
        if (*unit)
            unit[-1] = ' ';
        ui_text(8, (int32_t)(FONT_V.h - FONT_M.h) / 2, &FONT_M, b, fg, bg);
    }
    cv_blit(ui_tile_x(i), ui_tile_y(i) + UI_VAL_Y);
}

/* full: the whole tile; else the value and the bar only */
static void ui_knob_tile(uint32_t i, const ui_param_t* p, int lit, int full)
{
    const uint16_t kc = UI_KNOB_C[i], bg = lit ? kc : C_TILE;
    const uint16_t fg = lit && i == 2u ? RGB(10, 10, 12) : C_TEXT;   /* the white tile lit: dark text */
    const uint32_t x = ui_tile_x(i), y = ui_tile_y(i);
    if (full) {
        cv_begin(UI_TILE_W, UI_VAL_Y, bg);
        cv_rect(0, 0, UI_TILE_W, 4, kc);
        if (p)
            ui_text_cut(8, 9, &FONT_M, p->name, (int32_t)UI_TILE_W - 16, lit ? fg : C_SUB, bg);
        cv_blit(x, y);
        if (!p) {
            lcd_fill(x, y + UI_VAL_Y, UI_TILE_W, UI_TILE_H - UI_VAL_Y, bg);
            return;
        }
        lcd_fill(x, y + UI_VAL_Y + FONT_V.h, UI_TILE_W, UI_BAR_Y - UI_VAL_Y - FONT_V.h, bg);
        lcd_fill(x, y + UI_BAR_Y, 8, UI_TILE_H - UI_BAR_Y, bg);
        lcd_fill(x + UI_TILE_W - 8u, y + UI_BAR_Y, 8, UI_TILE_H - UI_BAR_Y, bg);
        lcd_fill(x + 8u, y + UI_BAR_Y + 4u, UI_TILE_W - 16u, UI_TILE_H - UI_BAR_Y - 4u, bg);
    }
    if (!p)
        return;
    ui_tile_value(i, p, bg, fg);
    ui_tile_bar(i, p, bg, lit ? fg : kc);
}

/* one knob's detents onto its parameter; 1 when the value moved */
static int ui_turn(const ui_param_t* p, int32_t detents)
{
    int32_t v;
    if (!p || !detents)
        return 0;
    v = *p->val + detents * p->step;
    v = v < p->min ? p->min : v > p->max ? p->max : v;
    if (v == *p->val)
        return 0;
    *p->val = v;
    if (p->apply)
        p->apply(v, p);
    return 1;
}

#endif
