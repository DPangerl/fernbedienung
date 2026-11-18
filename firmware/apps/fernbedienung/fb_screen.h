/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Daniel Pangerl */
/* Fernbedienung's screen (unity build, included once by app.c): Steckfeld's display on 240 x 240, in the regions of
 * fb_proto.h. Each region is drawn from a small description of what it shows; a region whose description did not
 * change since it was drawn is left alone, and the picture redraws at most every FB_PIC_MS on a new serial.
 *
 *   0..21     header: the module's title; the page's title and a dot per page; ARMED; the Edit / Route tag
 *   24..107   the picture (the kind's drawer, fb_pics_a.h / fb_pics_b.h / fb_pics_c.h), on Route the cables
 *   110..213  2 x 2 tiles: title, readout, where the control stands (and a tick where it plays when modulated)
 *   218..239  footer: what REC and SAVE do here, else the channel and the USB audio */
#ifndef FB_SCREEN_H
#define FB_SCREEN_H

#define FB_PIC_MS 33u                       /* ~30 pictures a second at most */
#define FB_ARMED_MS 400u                    /* ARMED blinks this fast */
#define FB_GRID 14                          /* the raster's pitch, as Steckfeld's display */

/* ---- themes: Steckfeld's display colours (App/UI/Theme.swift), pushed more saturated: at this panel's default
   gamma the iPad's values read pale (blue) and salmon (red). grid = ink at 18 %; ink_dim at 55 % */
typedef struct {
    uint8_t top[3], bottom[3];
    fb_theme_t c;
} fb_theme_src_t;

static const fb_theme_src_t FB_THEMES[2] = {
    /* light: blue phosphor; Route in the lamp's amber */
    { { 10, 60, 150 }, { 4, 30, 100 },
      { RGB(10, 60, 150), RGB(4, 30, 100), RGB(150, 225, 255), RGB(70, 140, 210), RGB(30, 80, 165),
        RGB(8, 18, 30), RGB(255, 176, 40) } },
    /* dark: red; Route amber on black */
    { { 40, 0, 0 }, { 18, 0, 0 },
      { RGB(40, 0, 0), RGB(18, 0, 0), RGB(255, 40, 20), RGB(160, 20, 10), RGB(75, 6, 3),
        RGB(0, 0, 0), RGB(255, 176, 40) } },
};

/* the hacker crews' screens (fb_crew, fb_pics_c.h): their own ground and inks, whatever the theme */
static const fb_theme_src_t FB_CREW_THEMES[4] = {
    /* ZKRAT: the Spectrum's black and blue */
    { { 3, 4, 16 }, { 3, 4, 16 },
      { RGB(3, 4, 16), RGB(3, 4, 16), RGB(220, 245, 255), RGB(92, 117, 255), RGB(18, 24, 64),
        RGB(8, 18, 30), RGB(255, 176, 40) } },
    /* LIGNE: poster paper */
    { { 235, 227, 207 }, { 222, 212, 190 },
      { RGB(235, 227, 207), RGB(222, 212, 190), RGB(18, 15, 18), RGB(110, 100, 92), RGB(205, 195, 172),
        RGB(8, 18, 30), RGB(255, 176, 40) } },
    /* KAIRO: night indigo */
    { { 5, 4, 16 }, { 12, 6, 32 },
      { RGB(5, 4, 16), RGB(12, 6, 32), RGB(255, 217, 242), RGB(60, 170, 190), RGB(32, 22, 62),
        RGB(8, 18, 30), RGB(255, 176, 40) } },
    /* RASTER: the drawing sheet */
    { { 14, 16, 20 }, { 14, 16, 20 },
      { RGB(14, 16, 20), RGB(14, 16, 20), RGB(232, 234, 238), RGB(140, 142, 148), RGB(44, 48, 56),
        RGB(8, 18, 30), RGB(255, 176, 40) } },
};

static const fb_theme_t* fb_th = &FB_THEMES[0].c;
static uint16_t fb_grow[240][3];            /* the gradient per screen row, R5 G6 B5 x 16 (dithered when drawn) */

static void fb_theme_set(uint32_t dark)
{
    const fb_theme_src_t* t = fb_crew ? &FB_CREW_THEMES[fb_crew - 1u] : &FB_THEMES[dark ? 1 : 0];
    static const uint32_t MAXC[3] = { 31u, 63u, 31u };
    uint32_t y, k;
    fb_th = &t->c;
    for (y = 0; y < 240u; y++)
        for (k = 0; k < 3u; k++) {
            const int32_t a = t->top[k], b = t->bottom[k];
            const int32_t v8x16 = a * 16 + (b - a) * 16 * (int32_t)y / 239;
            fb_grow[y][k] = (uint16_t)((uint32_t)v8x16 * MAXC[k] / 255u);
        }
}

/* the display's background at a screen pixel: the gradient, 4 x 4 ordered dither (no bands) */
static uint16_t fb_bg_px(uint32_t x, uint32_t y)
{
    static const uint8_t B[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };
    const uint32_t d = B[(y & 3u) * 4u + (x & 3u)];
    uint32_t r, g, b;
    if (y > 239u)
        y = 239u;
    r = (fb_grow[y][0] + d) >> 4;
    g = (fb_grow[y][1] + d) >> 4;
    b = (fb_grow[y][2] + d) >> 4;
    return (uint16_t)((r > 31u ? 31u : r) << 11 | (g > 63u ? 63u : g) << 5 | (b > 31u ? 31u : b));
}

/* the background behind text at screen row y (for its anti-aliasing) */
static uint16_t fb_bg_at(uint32_t y)
{
    if (y > 239u)
        y = 239u;
    return (uint16_t)(((fb_grow[y][0] + 8u) >> 4) << 11 | ((fb_grow[y][1] + 8u) >> 4) << 5 | ((fb_grow[y][2] + 8u) >> 4));
}

/* a canvas of w x h for screen x0, y0, filled with the display's background */
static void fb_cv(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h)
{
    uint32_t x, y;
    cv_begin(w, h, 0);
    for (y = 0; y < cv_h; y++)
        for (x = 0; x < w; x++)
            cv_px[y * w + x] = swap16(fb_bg_px(x0 + x, y0 + y));
}

static void fb_fill_bg(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h)
{
    fb_cv(x0, y0, w, h);
    cv_blit(x0, y0);
}

static int32_t fb_text_w(const felucca_font_t* f, const char* s, int32_t max_w)
{
    const int32_t w = text_w(f, s);
    return w < max_w ? w : max_w;
}

static void fb_frame_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    cv_rect(x, y, w, 1, c);
    cv_rect(x, y + h - 1, w, 1, c);
    cv_rect(x, y, 1, h, c);
    cv_rect(x + w - 1, y, 1, h, c);
}

/* ---- the picture region: the drawers start from this (fb_proto.h) */
static void fb_pic_begin(const fb_theme_t* th)
{
    int32_t x, y;
    fb_cv(FB_PIC_X, FB_PIC_Y, FB_PIC_W, FB_PIC_H);
    for (x = 8; x < (int32_t)FB_PIC_W; x += FB_GRID)
        cv_rect(x, 0, 1, (int32_t)FB_PIC_H, th->grid);
    for (y = 7; y < (int32_t)FB_PIC_H; y += FB_GRID)
        cv_rect(0, y, (int32_t)FB_PIC_W, 1, th->grid);
}

static const char* const FB_KIND_NAME[FB_K_COUNT] = {
    "Output", "Bridge", "Oscillator", "Wavetable", "String", "FM", "Filter", "LFO", "Envelope", "Delay", "Reverb",
    "Distortion", "Chorus", "EQ", "Compressor", "Grain", "Chord", "Arp", "Audio in", "MIDI", "Macro", "Sequencer", "Dump", "Combine", "Mage", "Smear"
};

/* two lines centred in the picture region: line 1 large, line 2 small and dim (either may be 0) */
static void fb_pic_message(const char* l1, const char* l2)
{
    const fb_theme_t* th = fb_th;
    const int32_t y1 = l2 ? 18 : 31, y2 = l1 ? 46 : 33;
    fb_pic_begin(th);
    if (l1)
        ui_text(((int32_t)FB_PIC_W - fb_text_w(&FONT_M, l1, 232)) / 2, y1, &FONT_M, l1, th->ink,
                fb_bg_at(FB_PIC_Y + (uint32_t)y1 + 11u));
    if (l2)
        ui_text_cut(((int32_t)FB_PIC_W - fb_text_w(&FONT_T, l2, 232)) / 2, y2, &FONT_T, l2, 232, th->ink_dim,
                fb_bg_at(FB_PIC_Y + (uint32_t)y2 + 8u));
    cv_blit(FB_PIC_X, FB_PIC_Y);
}

static void fb_pic_draw(const fb_pic_t* pic)
{
    if (fb_pics_a_draw(pic, fb_th) || fb_pics_b_draw(pic, fb_th) || fb_pics_c_draw(pic))
        return;
    fb_pic_message(pic->kind < FB_K_COUNT ? FB_KIND_NAME[pic->kind] : "", 0);
}

/* ---- Route: the cables as rows in the picture region, in the Route colours */
#define FB_ROUTE_ROW 21

static void fb_chevrons(int32_t x, int32_t y, int out, uint16_t c)
{
    int32_t i;
    for (i = 0; i < 2; i++) {
        const int32_t cx = x + 6 * i;
        if (out) {
            cv_line(cx, y, cx + 4, y + 4, c);
            cv_line(cx + 4, y + 4, cx, y + 8, c);
        } else {
            cv_line(cx + 4, y, cx, y + 4, c);
            cv_line(cx, y + 4, cx + 4, y + 8, c);
        }
    }
}

static void fb_route_draw(const fb_list_t* l)
{
    const fb_theme_t* th = fb_th;
    const uint16_t bg = th->route_bg, ink = th->route_ink, dim = ui_mix(bg, ink, 7), sel_bg = ui_mix(bg, ink, 3);
    const int32_t rows = (int32_t)FB_PIC_H / FB_ROUTE_ROW, s = (int32_t)l->sel - (int32_t)l->first;
    int32_t top = s - 1, i;
    cv_begin(FB_PIC_W, FB_PIC_H, bg);
    if (!l->open || !l->count) {
        const char* m = "No cables here";
        ui_text(((int32_t)FB_PIC_W - text_w(&FONT_M, m)) / 2, 31, &FONT_M, m, dim, bg);
        cv_blit(FB_PIC_X, FB_PIC_Y);
        return;
    }
    if (top > (int32_t)l->count - rows)
        top = (int32_t)l->count - rows;
    if (top < 0)
        top = 0;
    for (i = 0; i < rows && top + i < (int32_t)l->count; i++) {
        const fb_row_t* r = &l->row[top + i];
        const int on = r->flags & 1u, is_sel = top + i == s;
        const int32_t y = i * FB_ROUTE_ROW;
        const uint16_t rbg = is_sel ? sel_bg : bg, fg = on ? ink : dim;
        int32_t x, dw;
        if (is_sel) {
            cv_rect(0, y, (int32_t)FB_PIC_W, FB_ROUTE_ROW, rbg);
            fb_frame_rect(0, y, (int32_t)FB_PIC_W, FB_ROUTE_ROW, ink);
        }
        if (on)
            cv_rect(8, y + 7, 7, 7, ink);
        else
            fb_frame_rect(8, y + 7, 7, 7, dim);
        fb_chevrons(22, y + 6, r->flags & 2u, fg);
        dw = fb_text_w(&FONT_T, r->detail, 90);
        x = ui_text_cut(40, y + 2, &FONT_T, r->text, 228 - 40 - dw - 8, fg, rbg);
        (void)x;
        ui_text_cut(232 - dw, y + 2, &FONT_T, r->detail, 90, dim, rbg);
    }
    if (top > 0)
        cv_rect(236, 2, 2, 6, ink);         /* more above / below */
    if (top + rows < (int32_t)l->count)
        cv_rect(236, (int32_t)FB_PIC_H - 8, 2, 6, ink);
    cv_blit(FB_PIC_X, FB_PIC_Y);
}

/* ---- the header */
typedef struct {
    char title[FB_TITLE_N];
    char page[FB_LABEL_N];
    uint8_t cur, pages, route, armed;       /* armed: 0 not, 1 lit, 2 dim */
    uint8_t tag;                            /* 0 none, 1 EDIT, 2 ROUTE */
} fb_head_t;

static void fb_head_draw(const fb_head_t* h)
{
    const fb_theme_t* th = fb_th;
    const uint16_t bg = h->route ? th->route_ink : fb_bg_at(10), ink = h->route ? th->route_bg : th->ink;
    const uint16_t dim = h->route ? ui_mix(bg, ink, 9) : th->ink_dim;
    const int32_t armed_w = h->armed ? text_w(&FONT_T, "ARMED") + 8 + 8 : 0;
    int32_t x = 236, i, pw;
    if (h->route)
        cv_begin(240, FB_HEAD_H, bg);
    else
        fb_cv(0, FB_HEAD_Y, 240, FB_HEAD_H);
    if (h->tag) {
        const char* t = h->tag == 2u ? "ROUTE" : fb_crew ? pc_crew_name() : "EDIT";
        x -= text_w(&FONT_T, t);
        ui_text(x, 3, &FONT_T, t, h->route ? ink : fb_crew ? pc_crew_accent() : dim, bg);
        x -= 10;
    }
    if (h->pages > 1u) {
        for (i = (int32_t)h->pages - 1; i >= 0; i--) {
            x -= 6;
            cv_rect(x, 9, 5, 5, i == (int32_t)h->cur ? ink : dim);
            x -= 3;
        }
        x -= 4;
        pw = h->page[0] ? text_w(&FONT_T, h->page) : 0;
        if (pw && x - pw - 8 - armed_w >= 6 + text_w(&FONT_M, h->title) + 8) {   /* the page's name, where it fits */
            x -= pw;
            ui_text(x, 3, &FONT_T, h->page, ink, bg);
            x -= 8;
        }
    }
    if (h->armed) {                         /* blinks between a lit chip and its plain word */
        x -= armed_w - 8;
        if (h->armed == 1u)
            ui_chip(x, 2, "ARMED", ink, bg);
        else
            ui_text(x + 4, 2, &FONT_T, "ARMED", ink, bg);
        x -= 8;
    }
    ui_text_cut(6, 0, &FONT_M, h->title, x - 6, ink, bg);
    cv_blit(0, FB_HEAD_Y);
}

/* ---- a tile: KNOB i's title, readout and position */

static uint32_t fb_tile_x(uint32_t i) { return (i & 1u) * (240u - FB_TILE_W); }
static uint32_t fb_tile_y(uint32_t i) { return FB_TILE_Y + (i >> 1) * (FB_TILE_H + 4u); }

static void fb_tile_draw(uint32_t i, const fb_ctl_t* c)
{
    const fb_theme_t* th = fb_th;
    const uint32_t x0 = fb_tile_x(i), y0 = fb_tile_y(i);
    const int32_t bar_w = (int32_t)FB_TILE_W - 16;
    fb_cv(x0, y0, FB_TILE_W, FB_TILE_H);
    if (fb_crew) {
        if (c->flags & FB_CTL_PRESENT)
            pc_tile(i, c);
        cv_blit(x0, y0);
        return;
    }
    fb_frame_rect(0, 0, (int32_t)FB_TILE_W, (int32_t)FB_TILE_H, th->grid);
    if (c->flags & FB_CTL_PRESENT) {
        const felucca_font_t* f = text_w(&FONT_M, c->readout) <= (int32_t)FB_TILE_W - 16 ? &FONT_M : &FONT_T;
        ui_text_cut(8, 3, &FONT_T, c->title, (int32_t)FB_TILE_W - 14, th->ink_dim, fb_bg_at(y0 + 11u));
        ui_text_cut(8, f == &FONT_M ? 19 : 22, f, c->readout, (int32_t)FB_TILE_W - 14, th->ink, fb_bg_at(y0 + 30u));
        if (!(c->flags & FB_CTL_NOBAR)) {
            const int32_t p = (int32_t)((uint32_t)c->pos * (uint32_t)bar_w / FB_V_MAX);
            cv_rect(8, 44, bar_w, 3, th->grid);
            cv_rect(8, 44, p > 0 ? p : 1, 3, th->ink);
            if (c->flags & FB_CTL_LIVE) {
                const int32_t l = (int32_t)((uint32_t)c->live * (uint32_t)(bar_w - 2) / FB_V_MAX);
                cv_rect(8 + l, 40, 2, 10, th->ink);
            }
        }
    }
    cv_blit(x0, y0);
}

/* ---- the footer */
typedef struct {
    char l_chip[8], l_text[32];
    char r_chip[8], r_text[FB_LABEL_N + 8u];
    uint8_t notice;                         /* l_text is a notice: in the warning colour */
} fb_foot_t;

static void fb_foot_draw(const fb_foot_t* f)
{
    const fb_theme_t* th = fb_th;
    const uint16_t bg = fb_bg_at(FB_FOOT_Y + 11u);
    int32_t x = 6, rw = 0, rx;
    fb_cv(0, FB_FOOT_Y, 240, FB_FOOT_H);
    if (f->r_chip[0] || f->r_text[0]) {
        rw = (f->r_chip[0] ? text_w(&FONT_T, f->r_chip) + 8 + 6 : 0) + fb_text_w(&FONT_T, f->r_text, 110);
        rx = 234 - rw;
        if (f->r_chip[0])
            rx += ui_chip(rx, 3, f->r_chip, th->ink, bg) + 6;
        ui_text_cut(rx, 3, &FONT_T, f->r_text, 110, th->ink, bg);
    }
    if (f->l_chip[0])
        x += ui_chip(x, 3, f->l_chip, th->ink, bg) + 6;
    ui_text_cut(x, 3, &FONT_T, f->l_text, 234 - rw - 10 - x, f->notice ? C_WARN : f->l_chip[0] ? th->ink : th->ink_dim, bg);
    cv_blit(0, FB_FOOT_Y);
}

/* ---- the patch list over the picture and the tiles */
#define FB_OVL_Y FB_PIC_Y
#define FB_OVL_H (FB_FOOT_Y - FB_PIC_Y)     /* down to the footer: 8 rows */
#define FB_OVL_ROW 23

#define FB_SCROLL_PAUSE_MS 900u            /* a selected name too long for its row: still, across, still, back */
#define FB_SCROLL_PX_S 40u

/* the first row shown: the selection in the middle where it can be */
static int32_t fb_overlay_top(const fb_list_t* l)
{
    const int32_t rows = (int32_t)FB_OVL_H / FB_OVL_ROW, s = (int32_t)l->sel - (int32_t)l->first;
    int32_t top = s - rows / 2;
    if (top > (int32_t)l->count - rows)
        top = (int32_t)l->count - rows;
    return top < 0 ? 0 : top;
}

/* the room a row leaves its name: the detail and the SEQ chip on the right */
static int32_t fb_row_room(const fb_row_t* r)
{
    int32_t room = 232 - 8 - fb_text_w(&FONT_T, r->detail, 80) - 10;
    if (r->flags & FB_ROW_SEQ)
        room -= text_w(&FONT_T, "SEQ") + 8 + 6;
    return room;
}

static void fb_overlay_rows(const fb_list_t* l, int32_t strip_y)
{
    const fb_theme_t* th = fb_th;
    const int32_t rows = (int32_t)FB_OVL_H / FB_OVL_ROW, s = (int32_t)l->sel - (int32_t)l->first;
    const int32_t top = fb_overlay_top(l);
    int32_t i;
    cv_oy = -strip_y;
    if (!l->count)
        ui_text(8, 20, &FONT_M, "No patches", th->ink_dim, fb_bg_at(FB_OVL_Y + 30u));
    for (i = 0; i < rows && top + i < (int32_t)l->count; i++) {
        const fb_row_t* r = &l->row[top + i];
        const int32_t y = 3 + i * FB_OVL_ROW;
        const int is_sel = top + i == s;
        const uint16_t bg = is_sel ? th->ink : fb_bg_at(FB_OVL_Y + (uint32_t)y + 12u);
        const uint16_t fg = is_sel ? fb_bg_at(FB_OVL_Y + (uint32_t)y + 12u) : th->ink;
        const int32_t dw = fb_text_w(&FONT_T, r->detail, 80);
        if (is_sel)
            cv_rect(0, y, (int32_t)FB_PIC_W, FB_OVL_ROW, bg);
        ui_text_cut(8, y, &FONT_M, r->text, fb_row_room(r), fg, bg);
        if (r->flags & FB_ROW_SEQ)
            ui_chip(232 - dw - 6 - (text_w(&FONT_T, "SEQ") + 8), y + 3, "SEQ", is_sel ? fg : th->ink_dim, bg);
        ui_text_cut(232 - dw, y + 3, &FONT_T, r->detail, 80, is_sel ? fg : th->ink_dim, bg);
    }
    cv_oy = 0;
}

/* the selected name, scrolled when it does not fit; restart: the list was just drawn (the name at its start) */
static void fb_overlay_scroll(const fb_list_t* l, uint32_t ms, int restart)
{
    static uint32_t t0, shown;
    const int32_t s = (int32_t)l->sel - (int32_t)l->first, top = fb_overlay_top(l);
    const fb_row_t* r;
    int32_t room, span;
    uint32_t travel, t, x, y;
    if (restart) {
        t0 = ms;
        shown = 0;
    }
    if (s < 0 || s >= (int32_t)l->count)
        return;
    r = &l->row[s];
    room = fb_row_room(r);
    span = text_w(&FONT_M, r->text) - room;
    if (span <= 0)
        return;
    travel = (uint32_t)span * 1000u / FB_SCROLL_PX_S;
    t = (ms - t0) % (2u * FB_SCROLL_PAUSE_MS + travel);
    x = t < FB_SCROLL_PAUSE_MS ? 0u : t < FB_SCROLL_PAUSE_MS + travel ? (t - FB_SCROLL_PAUSE_MS) * FB_SCROLL_PX_S / 1000u
                                                                      : (uint32_t)span;
    if (x == shown)
        return;
    shown = x;
    y = 3u + (uint32_t)(s - top) * FB_OVL_ROW;
    cv_begin((uint32_t)room, FONT_M.h, fb_th->ink);
    ui_text(-(int32_t)x, 0, &FONT_M, r->text, fb_bg_at(FB_OVL_Y + y + 12u), fb_th->ink);
    cv_blit(8, FB_OVL_Y + y);
}

static void fb_overlay_draw(const fb_list_t* l)
{
    const uint32_t half = FB_OVL_H / 2u;
    fb_cv(0, FB_OVL_Y, 240, half);
    fb_overlay_rows(l, 0);
    cv_blit(0, FB_OVL_Y);
    fb_cv(0, FB_OVL_Y + half, 240, FB_OVL_H - half);
    fb_overlay_rows(l, (int32_t)half);
    cv_blit(0, FB_OVL_Y + half);
}

/* the strips between the regions (with a full redraw) */
static void fb_gaps(void)
{
    fb_fill_bg(0, FB_HEAD_H, 240, FB_PIC_Y - FB_HEAD_H);
    fb_fill_bg(0, FB_PIC_Y + FB_PIC_H, 240, FB_TILE_Y - FB_PIC_Y - FB_PIC_H);
    fb_fill_bg(FB_TILE_W, FB_TILE_Y, 240u - 2u * FB_TILE_W, 2u * FB_TILE_H + 4u);
    fb_fill_bg(0, FB_TILE_Y + FB_TILE_H, 240, 4);
    fb_fill_bg(0, FB_TILE_Y + 2u * FB_TILE_H + 4u, 240, FB_FOOT_Y - FB_TILE_Y - 2u * FB_TILE_H - 4u);
}

#endif
