/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Daniel Pangerl */
/* Fernbedienung's pictures, part B: LFO, envelope, sequencer, arp, chord, MIDI, macro, audio in, output, bridge, delay,
 * reverb. Steckfeld SF1's displays (App/UI/Displays) redrawn for the 240 x 84 picture region; what the app sends for
 * each is in docs/fernbedienung/pictures-b.md. Integer maths only (unity build: no FPU), included once by app.c after
 * fb_proto.h and bw_ui.h. Everything here is pb_*: part A lives in the same unity build. */
#ifndef FB_PICS_B_H
#define FB_PICS_B_H

#define PB_W 240
#define PB_H 84
#define PB_Q 16                             /* sub-pixel coordinates: 1/16 px */

static uint16_t pb_ink, pb_bot;             /* the theme's ink and its gradient's bottom (console: grey on black) */
static int16_t pb_lo[PB_W], pb_hi[PB_W];    /* a trace as a span per column */

/* ---- pixels */
static uint16_t pb_mix(uint16_t a, uint16_t b, uint32_t t)  /* t 0..256 */
{
    const uint32_t u = 256u - t;
    const uint32_t r = ((uint32_t)(a >> 11) * u + (uint32_t)(b >> 11) * t) >> 8;
    const uint32_t g = ((uint32_t)((a >> 5) & 63u) * u + (uint32_t)((b >> 5) & 63u) * t) >> 8;
    const uint32_t bl = ((uint32_t)(a & 31u) * u + (uint32_t)(b & 31u) * t) >> 8;
    return (uint16_t)(r << 11 | g << 5 | bl);
}

static void pb_blend(int32_t x, int32_t y, uint16_t c, uint32_t a)
{
    uint16_t* p;
    if ((uint32_t)x >= PB_W || (uint32_t)y >= PB_H || !a)
        return;
    p = &cv_px[(uint32_t)y * PB_W + (uint32_t)x];
    *p = swap16(a >= 256u ? c : pb_mix(swap16(*p), c, a));
}

static void pb_fill(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c, uint32_t a)
{
    int32_t i, j;
    for (j = y; j < y + h; j++)
        for (i = x; i < x + w; i++)
            pb_blend(i, j, c, a);
}

/* a 1 px frame; round: the corners left out (a radius of about 2) */
static void pb_frame(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c, uint32_t a, int round)
{
    const int32_t r = round ? 1 : 0;
    pb_fill(x + r, y, w - 2 * r, 1, c, a);
    pb_fill(x + r, y + h - 1, w - 2 * r, 1, c, a);
    pb_fill(x, y + 1, 1, h - 2, c, a);
    pb_fill(x + w - 1, y + 1, 1, h - 2, c, a);
}

static void pb_vdash(int32_t x, int32_t y0, int32_t y1, uint16_t c, uint32_t a)   /* 3 on, 3 off */
{
    int32_t y;
    for (y = y0; y < y1; y++)
        if ((y - y0) % 6 < 3)
            pb_blend(x, y, c, a);
}

static void pb_hdash(int32_t x0, int32_t x1, int32_t y, uint16_t c, uint32_t a)
{
    int32_t x;
    for (x = x0; x < x1; x++)
        if ((x - x0) % 6 < 3)
            pb_blend(x, y, c, a);
}

/* a ring between radii ri and ro (ri 0: a disc), centre and radii in 1/16 px, 4 x 4 samples a pixel */
static void pb_annulus(int32_t cx, int32_t cy, int32_t ro, int32_t ri, uint16_t c, uint32_t a)
{
    const int32_t ro2 = ro * ro, ri2 = ri * ri;
    int32_t x, y, sx, sy;
    for (y = (cy - ro) >> 4; y <= (cy + ro) >> 4; y++)
        for (x = (cx - ro) >> 4; x <= (cx + ro) >> 4; x++) {
            uint32_t n = 0;
            for (sy = 0; sy < 4; sy++)
                for (sx = 0; sx < 4; sx++) {
                    const int32_t dx = x * PB_Q + 2 + 4 * sx - cx, dy = y * PB_Q + 2 + 4 * sy - cy;
                    const int32_t d = dx * dx + dy * dy;
                    n += d <= ro2 && (!ri || d >= ri2);
                }
            pb_blend(x, y, c, a * n / 16u);
        }
}

/* the white dot riding a trace, its glow when lit */
static void pb_dot(int32_t x, int32_t y, int lit)
{
    if (lit)
        pb_annulus(x * PB_Q + 8, y * PB_Q + 8, 7 * PB_Q, 0, C_WHITE, 46);
    pb_annulus(x * PB_Q + 8, y * PB_Q + 8, 4 * PB_Q, 0, C_WHITE, 256);
}

/* text blended onto what is there, alpha a 0..256 */
static int32_t pb_text(int32_t x, int32_t y, const felucca_font_t* f, const char* s, uint16_t c, uint32_t a)
{
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
                    pb_blend(x - f->pad + (int32_t)gx, y + (int32_t)gy, c, a * v / 15u);
            }
        x += f->adv[gi];
    }
    return x;
}

/* Terminus (FONT_S): capitals on rows 2..11 of its 16 */
static void pb_label(int32_t x, int32_t cy, const char* s, uint16_t c, uint32_t a, int align)   /* 0 left, 1 centre, 2 right */
{
    const int32_t w = text_w(&FONT_S, s);
    pb_text(align == 1 ? x - w / 2 : align == 2 ? x - w : x, cy - 7, &FONT_S, s, c, a);
}

/* ---- the display: Steckfeld's gradient and its 12 px raster; console: black (the macro's text mode) */
static uint16_t pb_grey(uint16_t c)
{
    const uint32_t l = (((uint32_t)(c >> 11) * 527u >> 6) * 77u + ((uint32_t)((c >> 5) & 63u) * 259u >> 6) * 150u
                        + ((uint32_t)(c & 31u) * 527u >> 6) * 29u) >> 8;
    return RGB(l, l, l);
}

static void pb_begin(const fb_theme_t* th, int console)
{
    int32_t x, y;
    cv_oy = 0;
    cv_begin(PB_W, PB_H, console ? C_BLACK : th->bg_top);
    if (console) {
        pb_ink = pb_grey(th->ink);
        pb_bot = pb_grey(th->bg_bottom);
        return;
    }
    pb_ink = th->ink;
    pb_bot = th->bg_bottom;
    for (y = 1; y < PB_H; y++) {
        const uint16_t c = swap16(pb_mix(th->bg_top, th->bg_bottom, (uint32_t)y * 256u / (PB_H - 1)));
        for (x = 0; x < PB_W; x++)
            cv_px[y * PB_W + x] = c;
    }
    for (x = 12; x < PB_W; x += 12)
        for (y = 0; y < PB_H; y++)
            cv_px[y * PB_W + x] = swap16(th->grid);
    for (y = 12; y < PB_H; y += 12)
        for (x = 0; x < PB_W; x++)
            cv_px[y * PB_W + x] = swap16(th->grid);
}

/* ---- traces: a path as one span per column (every path here runs left to right), then hatched and drawn glowing */
static void pb_span_clear(void)
{
    int32_t x;
    for (x = 0; x < PB_W; x++) {
        pb_lo[x] = 0x7FFF;
        pb_hi[x] = -0x7FFF;
    }
}

static void pb_span_pt(int32_t x, int32_t y16)
{
    const int32_t y = (y16 + 8) >> 4;
    if ((uint32_t)x >= PB_W)
        return;
    if (y < pb_lo[x])
        pb_lo[x] = (int16_t)y;
    if (y > pb_hi[x])
        pb_hi[x] = (int16_t)y;
}

static void pb_seg(int32_t xa, int32_t ya, int32_t xb, int32_t yb)   /* 1/16 px */
{
    int32_t x, x0, x1;
    if (xa > xb) {
        x = xa, xa = xb, xb = x;
        x = ya, ya = yb, yb = x;
    }
    if (xa == xb) {
        pb_span_pt(xa >> 4, ya);
        pb_span_pt(xa >> 4, yb);
        return;
    }
    x0 = (xa >> 4) < 0 ? 0 : xa >> 4;
    x1 = (xb >> 4) >= PB_W ? PB_W - 1 : xb >> 4;
    for (x = x0; x <= x1; x++) {
        const int32_t l = x * PB_Q > xa ? x * PB_Q : xa, r = x * PB_Q + 15 < xb ? x * PB_Q + 15 : xb;
        pb_span_pt(x, ya + (yb - ya) * (l - xa) / (xb - xa));
        pb_span_pt(x, ya + (yb - ya) * (r - xa) / (xb - xa));
    }
}

/* Steckfeld's hatch: upright lines every 3 px between the trace and base */
static void pb_hatch(int32_t base, uint32_t a)
{
    int32_t x, y;
    for (x = 0; x < PB_W; x += 3) {
        if (pb_lo[x] > pb_hi[x])
            continue;
        if (pb_hi[x] < base)
            for (y = pb_hi[x] + 1; y <= base; y++)
                pb_blend(x, y, pb_ink, a);
        else if (pb_lo[x] > base)
            for (y = base; y < pb_lo[x]; y++)
                pb_blend(x, y, pb_ink, a);
    }
}

/* strokeGlowing: the line, and around it the glow fading over 3 px (each pixel blended once) */
static void pb_trace(uint32_t a, int glow)
{
    static const uint8_t HALO[4] = { 0, 92, 48, 22 };
    const int32_t reach = glow ? 3 : 0;
    int32_t x, y, d, i;
    for (x = 0; x < PB_W; x++) {
        int32_t top[4], bot[4];
        for (d = 0; d <= reach; d++) {
            top[d] = 0x7FFF;
            bot[d] = -0x7FFF;
            for (i = x - d; i <= x + d; i++)
                if (i >= 0 && i < PB_W && pb_lo[i] <= pb_hi[i]) {
                    if (pb_lo[i] - d < top[d])
                        top[d] = pb_lo[i] - d;
                    if (pb_hi[i] + d > bot[d])
                        bot[d] = pb_hi[i] + d;
                }
        }
        for (y = top[reach]; y <= bot[reach]; y++) {
            for (d = 0; d <= reach && (y < top[d] || y > bot[d]); d++)
                ;
            pb_blend(x, y, pb_ink, d ? HALO[d] * a >> 8 : a);
        }
    }
}

/* ---- fixed point */
static uint32_t pb_u16(const fb_pic_t* p, uint32_t i)    /* v[i], 0..16383 as 0..65536 (Q16) */
{
    const uint32_t v = i < p->n ? p->v[i] : 0u;
    return v >= FB_V_MAX ? 65536u : v << 2;
}

static int32_t pb_v(const fb_pic_t* p, uint32_t i) { return i < p->n ? (int32_t)p->v[i] : 0; }
static int32_t pb_live(const fb_pic_t* p, uint32_t i) { return i < p->live_n ? (int32_t)p->live[i] : 0; }

/* 2^x, x and the result Q16 */
static uint32_t pb_exp2(int32_t x)
{
    const int32_t i = x >> 16;
    const uint32_t f = (uint32_t)x & 0xFFFFu;
    uint32_t p = 5125u;                                  /* 1 + .6957 f + .2262 f^2 + .0782 f^3 */
    p = 14824u + (p * f >> 16);
    p = 45594u + (p * f >> 16);
    p = 65536u + (p * f >> 16);
    if (i >= 15)
        return 0xFFFFFFFFu;
    return i >= 0 ? p << i : i > -31 ? p >> -i : 0u;
}

/* log2 of a Q16 value, Q16 (about 0.005 off at most) */
static int32_t pb_log2(uint32_t v)
{
    uint32_t b = 0, f;
    if (!v)
        return -32 * 65536;
    while (b < 31u && (v >> (b + 1u)))
        b++;
    f = b >= 16u ? (v >> (b - 16u)) & 0xFFFFu : (v << (16u - b)) & 0xFFFFu;
    return ((int32_t)b - 16) * 65536 + (int32_t)(f + (((f * (65536u - f)) >> 16) * 22u >> 6));
}

static uint32_t pb_isqrt(uint32_t v)
{
    uint32_t r = 0, bit = 1u << 30;
    while (bit > v)
        bit >>= 2;
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

static void pb_note_name(char* b, int32_t note)
{
    static const char NAMES[] = "C C#D D#E F F#G G#A A#B ";
    const int32_t o = note / 12 - 1;
    uint32_t i = 0;
    b[i++] = NAMES[note % 12 * 2];
    if (NAMES[note % 12 * 2 + 1] == '#')
        b[i++] = '#';
    if (o < 0)
        b[i++] = '-';
    b[i++] = (char)('0' + (o < 0 ? -o : o));
    b[i] = 0;
}

/* ================================================================ the kinds */

/* ---- LFO (LFODisplay over WaveformDisplay): the wave running past, the now needle in the middle */
static const int16_t PB_STAIRS[8] = { 9830, -22938, 27853, -6554, 18022, -29491, 3277, 22938 };

static int32_t pb_wave(int32_t wave, uint32_t phase)       /* phase Q16 cycles; Q15 out */
{
    const int32_t t = (int32_t)(phase & 0xFFFFu);
    int32_t from, to, c;
    switch (wave) {
    case 0:
        return FW_SINE[t >> 6];
    case 1:
        c = 32768 - 2 * (t > 32768 ? t - 32768 : 32768 - t);
        return c > 32767 ? 32767 : c;
    case 3:
        return t < 32768 ? 32767 : -32767;
    case 4:
        return PB_STAIRS[t >> 13];
    case 5:
        return ((int32_t)((((uint32_t)(t * 48) >> 16) * 2654435761u >> 16) % 2000u) - 1000) * 32767 / 1000;
    case 6:
        from = PB_STAIRS[((phase >> 16) - 1u) & 7u];
        to = PB_STAIRS[(phase >> 16) & 7u];
        c = 16384 - FW_SINE[(((uint32_t)t >> 1) + 16384u) >> 6 & 1023u] / 2;   /* 0.5 - 0.5 cos(pi t) */
        return from + (to - from) * c / 32768;
    default:
        return t - 32768;
    }
}

static void pb_lfo(const fb_pic_t* p)
{
    const int32_t wave = pb_v(p, 0), mid = PB_H / 2;
    const int32_t amp = (int32_t)(pb_u16(p, 1) * (uint32_t)(mid - 6) >> 12);  /* 1/16 px */
    const uint32_t cycles = (uint32_t)(pb_v(p, 2) < 256 ? 256 : pb_v(p, 2)) << 8, scroll = (uint32_t)pb_live(p, 0) << 5;
    int32_t i, y0 = 0, y;
    pb_span_clear();
    for (i = 0; i <= 2 * PB_W; i++) {
        y = mid * PB_Q - (amp * pb_wave(wave, scroll + cycles * (uint32_t)i / (2u * PB_W)) >> 15);
        if (i)
            pb_seg((i - 1) * 8, y0, i * 8, y);
        y0 = y;
    }
    pb_hatch(mid, 56);
    pb_trace(256, 1);
    pb_vdash(PB_W / 2, 0, PB_H, pb_ink, 102);
    y = mid * PB_Q - (amp * pb_wave(wave, scroll + cycles / 2u) >> 15);
    pb_dot(PB_W / 2, (y + 8) >> 4, 1);
}

/* ---- envelope (EnvelopeDisplay): ADSR on a fixed time axis, the engine's curved stages */
static int32_t pb_px, pb_py;                                /* the path's pen, 1/16 px */

static void pb_line_to(int32_t x, int32_t y)
{
    pb_seg(pb_px, pb_py, x, y);
    pb_px = x;
    pb_py = y;
}

/* a stage aiming 10^(2 - k curve) past its end (k: 4.6 rising, 8 falling) and landing on time */
static void pb_stage(int32_t bx, int32_t by, uint32_t curve, int32_t k10)
{
    const int32_t ax = pb_px, ay = pb_py;
    const int32_t l = (int32_t)(((int64_t)(131072 - (int32_t)(curve * (uint32_t)k10 / 10u)) * 217706) >> 16);
    const uint32_t r = l < -24 * 65536 ? 0u : pb_exp2(l);
    const int32_t lq = l - pb_log2(65536u + r);              /* log2 (r / (1 + r)) */
    int32_t i;
    for (i = 1; i <= 24; i++) {
        const uint32_t e = pb_exp2(lq * i / 24);
        int32_t done = (int32_t)(((uint64_t)(65536u + r) * (e < 65536u ? 65536u - e : 0u)) >> 16);
        done = done > 65536 ? 65536 : done;
        pb_line_to(ax + (bx - ax) * i / 24, ay + (int32_t)((int64_t)(by - ay) * done >> 16));
    }
}

static void pb_envelope(const fb_pic_t* p)
{
    const int32_t stage = PB_W * 3 / 4 / 3;                /* sustain holds a quarter, each timed stage a third of the rest */
    const int32_t top = 4 * PB_Q, bottom = (PB_H - 2) * PB_Q;
    const uint32_t curve = pb_u16(p, 4);
    int32_t w[3], i, ax, dx, rx, ex, sy;
    for (i = 0; i < 3; i++) {
        w[i] = (int32_t)(pb_u16(p, i == 2 ? 3u : (uint32_t)i) * (uint32_t)(stage * PB_Q) >> 16);
        w[i] = w[i] < 3 * PB_Q ? 3 * PB_Q : w[i];
    }
    ax = w[0];
    dx = ax + w[1];
    rx = dx + PB_W * PB_Q / 4;
    ex = rx + w[2];
    sy = bottom - (int32_t)((uint32_t)(bottom - top) * pb_u16(p, 2) >> 16);
    pb_span_clear();
    pb_px = 0;
    pb_py = bottom;
    pb_stage(ax, top, curve, 46);
    pb_stage(dx, sy, curve, 80);
    pb_line_to(rx, sy);
    pb_stage(ex, bottom, curve, 80);
    pb_hatch(PB_H - 2, 56);
    pb_vdash(rx >> 4, 0, PB_H, pb_ink, 102);
    pb_trace(256, 1);
    pb_fill((ax >> 4) - 2, (top >> 4) - 2, 5, 5, C_WHITE, 256);
    pb_fill((dx >> 4) - 2, (sy >> 4) - 2, 5, 5, C_WHITE, 256);
    pb_fill((rx >> 4) - 2, (sy >> 4) - 2, 5, 5, C_WHITE, 256);
}

/* ---- sequencer (StepNotesDisplay, keys): 4 rows of 8, note names, a gate bar each, the running step lit */
#define PB_SEQ_STEPS 32

static void pb_sequencer(const fb_pic_t* p)
{
    const int32_t len = pb_v(p, 0) ? pb_v(p, 0) : PB_SEQ_STEPS, cursor = p->n > 1u ? pb_v(p, 1) : -1;
    const int32_t head = pb_live(p, 0) - 1, cw = 28, ch = 19, gut = 2;
    int32_t i;
    for (i = 0; i < PB_SEQ_STEPS; i++) {
        const int32_t fx = 1 + i % 8 * (cw + gut), fy = 1 + i / 8 * (ch + gut);
        const uint8_t* s = &p->arr[3 * i];
        const int has = (uint32_t)(3 * i + 2) < p->arr_n, on = has && (s[1] & 1u);
        const int cur = i == head, in = i < len, plays = on && in;
        const uint16_t ink = cur ? pb_bot : pb_ink;
        const uint32_t ia = cur || plays ? 256u : 102u;
        char name[6];
        if (cur) {
            pb_fill(fx + 1, fy, cw - 2, ch, pb_ink, 256);
            pb_fill(fx, fy + 1, 1, ch - 2, pb_ink, 256);
            pb_fill(fx + cw - 1, fy + 1, 1, ch - 2, pb_ink, 256);
        } else {
            pb_fill(fx, fy, cw, ch, pb_ink, plays ? 20u : 8u);
            pb_frame(fx, fy, cw, ch, pb_ink, plays ? 90u : in ? 38u : 20u, 1);
        }
        if (i == cursor)
            pb_frame(fx - 1, fy - 1, cw + 2, ch + 2, pb_ink, 256, 1);
        if (!in)
            continue;
        if (on)
            pb_note_name(name, s[0]);
        else
            name[0] = '-', name[1] = 0;
        pb_text(fx + (cw - text_w(&FONT_S, name)) / 2, fy + (on ? 0 : 2), &FONT_S, name, ink, ia);
        if (on) {
            const int32_t lx = fx + 4, lw = cw - 8, ly = fy + ch - 5, rep = 1 + (s[1] >> 3 & 3u), g = s[1] >> 1 & 3u;
            const int32_t hit = lw * PB_Q / rep, fill = g == 3 ? hit : (hit * (g + 1) / 4 - 24 > 32 ? hit * (g + 1) / 4 - 24 : 32);
            const uint32_t bright = 102u + 154u * s[2] / 127u;
            int32_t k, x;
            pb_fill(lx, ly, lw, 3, ink, 38);
            for (k = 0; k < rep; k++)
                for (x = (lx * PB_Q + k * hit) >> 4; x < (lx * PB_Q + k * hit + fill) >> 4; x++)
                    pb_fill(x, ly, 1, 3, ink, bright);
        }
    }
}

/* ---- arp (ArpDisplay): 16 steps over a chord of three, a band per octave */
static int32_t pb_arp_note(int32_t mode, int32_t s, int32_t n)
{
    static const uint8_t PLAYED[3] = { 2, 0, 1 };
    int32_t period, turn;
    switch (mode) {
    case 1:
        return n - 1 - s % n;
    case 2:
        period = 2 * n - 2 > 1 ? 2 * n - 2 : 1;
        turn = s % period;
        return turn < n ? turn : period - turn;
    case 3:
        return PLAYED[s % 3] + s % n / 3 * 3;
    case 4:
        return (s * 7 + s * s * 3) % n;
    default:
        return s % n;
    }
}

static void pb_arp(const fb_pic_t* p)
{
    const int32_t oct = pb_v(p, 1) < 1 ? 1 : pb_v(p, 1) > 4 ? 4 : pb_v(p, 1), n = 3 * oct;
    const int32_t top = 8, h = PB_H - 16, col = PB_W / 16, row = h * PB_Q / n;   /* row: 1/16 px */
    const int32_t gw = (int32_t)((uint32_t)col * pb_u16(p, 2) >> 16) - 2;
    int32_t i;
    for (i = 1; i < oct; i++)
        pb_hdash(0, PB_W, top + h - i * 3 * row / PB_Q, pb_ink, 102);
    for (i = 0; i < 16; i++) {
        const int32_t y = top * PB_Q + h * PB_Q - (pb_arp_note(pb_v(p, 0), i, n) + 1) * row;
        pb_fill(i * col + 1, (y >> 4) + 1, gw < 2 ? 2 : gw, row / PB_Q - 2 < 2 ? 2 : row / PB_Q - 2, pb_ink, 256);
    }
}

/* ---- chord (ChordDisplay): the octave above the note, a lane a semitone, the chord's notes lit */
static void pb_chord(const fb_pic_t* p)
{
    static const uint16_t CHORDS[8] = { 0x0001, 0x0081, 0x1001, 0x0089, 0x0091, 0x00A1, 0x0489, 0x0891 };
    const int32_t top = 8, h = PB_H - 16, row = h * PB_Q / 13;
    const uint16_t set = CHORDS[pb_v(p, 0) & 7];
    int32_t i;
    for (i = 1; i < 13; i++)
        pb_fill(0, top + i * row / PB_Q, PB_W, 1, pb_ink, 51);
    for (i = 0; i < 13; i++)
        if (set >> i & 1u) {
            const int32_t y = (top * PB_Q + h * PB_Q - (i + 1) * row) >> 4;
            pb_fill(2, y + 1, PB_W - 4, row / PB_Q - 2 < 2 ? 2 : row / PB_Q - 2, pb_ink, 256);
        }
}

/* ---- MIDI (MidiDisplay): the DIN socket lit while notes come in, the channels, the keyboard with Bend's reach */
static void pb_midi(const fb_pic_t* p)
{
    static const int8_t PIN_X[5] = { -62, -44, 0, 44, 62 }, PIN_Y[5] = { 12, -32, -50, -32, 12 };   /* 0.62 r (cos, sin) + 0.12 r, %  */
    static const uint8_t WHITE[7] = { 0, 2, 4, 5, 7, 9, 11 }, BLACK_S[5] = { 1, 3, 6, 8, 10 }, BLACK_W[5] = { 1, 2, 4, 5, 6 };
    const int32_t ch = pb_v(p, 0), bend = pb_v(p, 1), active = pb_live(p, 0) != 0;
    const int32_t th = PB_H * 56 / 100, r = (th * PB_Q) / 2 - 4 * PB_Q, cx = (th * PB_Q) / 2 + 10 * PB_Q, cy = th * PB_Q / 2;
    const int32_t ky = PB_H * 62 / 100, kh = PB_H - ky, kw = PB_W * 256 / 29;               /* kw: 1/256 px */
    int32_t i;
    if (active)
        pb_annulus(cx, cy, r + 4 * PB_Q, 0, C_WHITE, 46);
    pb_annulus(cx, cy, r + PB_Q, r - PB_Q, pb_ink, 256);
    pb_fill((cx - r * 12 / 100) >> 4, (cy + r * 78 / 100) >> 4, (r * 24 / 100 + 15) >> 4, (r * 22 / 100 + 15) >> 4, pb_ink, 256);
    for (i = 0; i < 5; i++) {
        const int32_t px = cx + r * PIN_X[i] / 100, py = cy + r * PIN_Y[i] / 100, pr = r / 10;
        if (active)
            pb_annulus(px, py, pr, 0, C_WHITE, 256);
        else
            pb_annulus(px, py, pr + 8, pr - 8, pb_ink, 256);
    }
    /* the channels: 2 rows of 8 (Steckfeld: 4 x 4, too small for its numbers here); two digits set 7 px apart */
    for (i = 0; i < 16; i++) {
        const int32_t x0 = th + 16, cwid = (PB_W - x0) * PB_Q / 8, bx = (x0 * PB_Q + i % 8 * cwid) >> 4, by = 4 + i / 8 * 20;
        const int32_t bw = cwid / PB_Q - 3, on = ch == 0 || ch == i + 1, mid = bx + bw / 2 + 1;
        const uint16_t c = on ? pb_bot : pb_ink;
        const uint32_t a = on ? 256u : 102u;
        char d[2] = { 0, 0 };
        if (on)
            pb_fill(bx, by, bw, 17, pb_ink, 256);
        else
            pb_frame(bx, by, bw, 17, pb_ink, 102, 0);
        if (i + 1 >= 10) {
            d[0] = '1';
            pb_label(mid - 4, by + 8, d, c, a, 1);
            d[0] = (char)('0' + i - 9);
            pb_label(mid + 3, by + 8, d, c, a, 1);
        } else {
            d[0] = (char)('1' + i);
            pb_label(mid, by + 8, d, c, a, 1);
        }
    }
    /* four octaves, C to C: Bend's reach either side of the middle C */
    for (i = 0; i <= 28; i++) {
        const int32_t semi = i / 7 * 12 + WHITE[i % 7], x0 = i * kw >> 8, x1 = (i + 1) * kw >> 8;
        if (semi >= 24 - bend && semi <= 24 + bend)
            pb_fill(x0, ky, x1 - x0, kh, pb_ink, semi == 24 ? 256u : 141u);
        pb_fill(x0, ky, 1, kh, pb_ink, 180);
    }
    pb_fill(0, ky, PB_W, 1, pb_ink, 180);
    pb_fill(PB_W - 1, ky, 1, kh, pb_ink, 180);
    for (i = 0; i < 20; i++) {
        const int32_t o = i / 5, k = i % 5, semi = o * 12 + BLACK_S[k];
        const int32_t xc = (o * 7 + BLACK_W[k]) * kw, x0 = (xc - kw * 3 / 10) >> 8, w = (kw * 6 / 10) >> 8, h = kh * 6 / 10;
        pb_fill(x0, ky, w + 1, h, pb_bot, 256);
        if (semi >= 24 - bend && semi <= 24 + bend)
            pb_fill(x0, ky, w + 1, h, pb_ink, 141);
        pb_frame(x0, ky, w + 1, h, pb_ink, 180, 0);
    }
}

/* ---- macro (MacroDisplay), Kabelmaier's console at its tidiest, grey on black: the four encoders side by side,
   each a tower of its steps from step 1 at the bottom; a line between written steps (the fade), a notch cut into a
   step where a choice switches (no fade into it), the cursor framed,
   the free slot dashed, and a pointer where the encoder stands between its steps */
#define PB_MAC_STEPS 4

static void pb_cat(char* b, const char* s)
{
    while (*b)
        b++;
    while (*s)
        *b++ = *s++;
    *b = 0;
}

static void pb_cat_int(char* b, int32_t v)
{
    char n[12];
    fmt_int(n, v);
    pb_cat(b, n);
}

static void pb_macro(const fb_pic_t* p)
{
    const int32_t enc = pb_v(p, 0) & 3, unsaved = pb_v(p, 2) & 1;
    char b[8];
    int32_t i, k;
    pb_fill(0, 0, PB_W, 12, pb_ink, 256);
    pb_text(0, -1, &FONT_S, " MK-4 MACRO", pb_bot, 256);
    pb_label(PB_W - 8, 6, unsaved ? "UNSAVED" : "READY", pb_bot, 256, 2);
    for (i = 0; i < 4; i++) {
        const int32_t count = pb_v(p, 3u + (uint32_t)i) > PB_MAC_STEPS ? PB_MAC_STEPS : pb_v(p, 3u + (uint32_t)i);
        const int32_t cx = 30 + i * 60, x = cx - 14, sel = i == enc, cuts = pb_v(p, 11u + (uint32_t)i);
        const uint32_t a = sel ? 256u : 150u;
        int32_t step = pb_v(p, 1);
        step = step > count ? count : step;
        step = step >= PB_MAC_STEPS ? PB_MAC_STEPS - 1 : step;
        for (k = 0; k < PB_MAC_STEPS; k++) {               /* step k: a 9 px cell, 13 px apart */
            const int32_t y = 62 - k * 13;
            if (k < count) {
                pb_fill(x, y, 28, 9, pb_ink, a);
                if (cuts >> k & 1)
                    pb_fill(cx - 3, y, 6, 3, C_BLACK, 256);
                if (k + 1 < count && !(cuts >> (k + 1) & 1))
                    pb_fill(cx - 1, y - 4, 2, 4, pb_ink, a);
            } else if (sel && k == step) {
                pb_hdash(x, x + 28, y, pb_ink, a);
                pb_hdash(x, x + 28, y + 8, pb_ink, a);
                pb_vdash(x, y, y + 9, pb_ink, a);
                pb_vdash(x + 27, y, y + 9, pb_ink, a);
            } else {
                pb_fill(cx - 1, y + 4, 2, 2, pb_ink, a);
            }
            if (sel && k == step)
                pb_frame(x - 3, y - 2, 34, 13, pb_ink, 256, 0);
        }
        if (count) {                                        /* the pointer: 1.0 at step 1, count.0 at the top */
            const int32_t y = 66 - (int32_t)((pb_u16(p, 7u + (uint32_t)i) * (uint32_t)(count - 1) * 13u + 32768u) >> 16);
            pb_fill(x - 9, y - 2, 1, 5, pb_ink, a);
            pb_fill(x - 8, y - 1, 1, 3, pb_ink, a);
            pb_fill(x - 7, y, 1, 1, pb_ink, a);
        }
        b[0] = (char)('A' + i);
        b[1] = 0;
        if (sel) {
            pb_fill(cx - 8, 73, 16, 11, pb_ink, 256);
            pb_label(cx, 79, b, C_BLACK, 256, 1);
        } else {
            pb_label(cx, 79, b, pb_ink, a, 1);
        }
    }
}

/* ---- audio in (AudioInDisplay): a jack socket and the level going in, or word that the input is not allowed */
static void pb_audio_in(const fb_pic_t* p)
{
    const int32_t r = PB_H * 22 * PB_Q / 100, cx = PB_W * PB_Q / 2, cy = PB_H * 34 * PB_Q / 100, denied = pb_v(p, 1) & 1;
    const int32_t by = 56, bh = 12;
    pb_annulus(cx, cy, r + PB_Q, r - PB_Q, pb_ink, 256);
    pb_annulus(cx, cy, r * 62 / 100 + 8, r * 62 / 100 - 8, pb_ink, 256);
    pb_annulus(cx, cy, r * 24 / 100, 0, denied ? pb_ink : C_WHITE, denied ? 102u : 256u);
    pb_frame(0, by, PB_W, bh, pb_ink, 256, 0);
    pb_fill(0, by, (int32_t)(pb_u16(p, 0) * PB_W >> 16), bh, pb_ink, 256);
    if (p->live_n)                                          /* modulated: how it plays now, a ghost over it */
        pb_fill(0, by, (int32_t)((uint32_t)pb_live(p, 0) * PB_W / FB_V_MAX), bh, pb_ink, 115);
    if (denied)
        pb_label(PB_W / 2, 77, "NO MICROPHONE ACCESS", pb_ink, 256, 1);
}

/* ---- output (AmpDisplay): the output level running past, four seconds (120 readings) to the screen, the held peak */
#define PB_AMP_N 120
static uint16_t pb_amp_hist[PB_AMP_N];
static uint32_t pb_amp_serial;

static void pb_output(const fb_pic_t* p)
{
    static const char* const DB[5] = { "-48", "-36", "-24", "-12", "0" };
    const int32_t rx = 30, rw = PB_W - 4 - rx, ry = 6, rh = PB_H - 12;
    int32_t i, y0 = 0;
    if (p->live_n >= 2u && p->serial != pb_amp_serial) {   /* a LIVE message is one reading */
        for (i = 0; i < PB_AMP_N - 1; i++)
            pb_amp_hist[i] = pb_amp_hist[i + 1];
        pb_amp_hist[PB_AMP_N - 1] = (uint16_t)(p->live[0] > FB_V_MAX ? FB_V_MAX : p->live[0]);
        pb_amp_serial = p->serial;
    }
    for (i = 0; i < 5; i++) {
        const int32_t y = ry + rh - rh * i / 4;
        pb_fill(rx - 4, y, 4, 1, pb_ink, 102);
        pb_label(rx - 6, y, DB[i], pb_ink, 102, 2);
    }
    pb_span_clear();
    for (i = 0; i < PB_AMP_N; i++) {
        const int32_t y = (ry + rh) * PB_Q - (int32_t)((uint32_t)rh * PB_Q * pb_amp_hist[i] / FB_V_MAX);
        if (i)
            pb_seg(rx * PB_Q + rw * PB_Q * (i - 1) / (PB_AMP_N - 1), y0, rx * PB_Q + rw * PB_Q * i / (PB_AMP_N - 1), y);
        y0 = y;
    }
    pb_hatch(ry + rh, 56);
    pb_trace(256, 1);
    pb_hdash(rx, rx + rw, ry + rh - rh * pb_live(p, 1) / (int32_t)FB_V_MAX, pb_ink, 102);
    pb_dot(rx + rw, (y0 + 8) >> 4, 1);
}

/* ---- bridge (BridgeDisplay): cables in on the left, out on the right, through a node carrying the mode */
static const char* pb_str(const fb_pic_t* p, uint32_t* at)    /* the next str in arr; "" past its end */
{
    const char* s = (const char*)&p->arr[*at];
    if (*at >= p->arr_n)
        return "";
    while (*at < p->arr_n && p->arr[*at])
        (*at)++;
    if (*at >= p->arr_n)
        return "";                                          /* not ended: dropped */
    (*at)++;
    return s;
}

static void pb_cable(int32_t x, int32_t y, int32_t nx, int32_t ny, uint32_t a)   /* 1/16 px */
{
    const int32_t mx = (x + nx) / 2;
    int32_t k, px = x, py = y;
    pb_span_clear();
    for (k = 1; k <= 32; k++) {                             /* the curve, its controls level with either end */
        const int32_t u = 32 - k, w0 = u * u * u, w1 = 3 * u * u * k, w2 = 3 * u * k * k, w3 = k * k * k;
        const int32_t qx = (w0 * x + w1 * mx + w2 * mx + w3 * nx) >> 15, qy = (w0 * y + w1 * y + w2 * ny + w3 * ny) >> 15;
        pb_seg(px, py, qx, qy);
        px = qx;
        py = qy;
    }
    pb_trace(a, 1);
}

static void pb_bridge(const fb_pic_t* p)
{
    static const char* const MODE[4] = { "OPEN", "THRU", "SPLIT", "MERGE" };
    const int32_t nin = pb_v(p, 0) > 8 ? 8 : pb_v(p, 0), nout = pb_v(p, 1) > 8 ? 8 : pb_v(p, 1);
    const int32_t nx = PB_W / 2, ny = PB_H / 2, r = 22, room = (nx - r - 6) / 8;   /* characters before the node */
    char names[16][16];
    int32_t lx[16], ly[16], al[16];
    uint32_t la[16], at = 0;
    int32_t side, i, k = 0;
    for (side = 0; side < 2; side++) {
        const int32_t n = side ? nout : nin, x = side ? PB_W : 0;
        if (!n)
            pb_hdash(side ? nx : 0, side ? PB_W : nx, ny, pb_ink, 102);
        for (i = 0; i < n; i++) {
            const int32_t y = PB_H * PB_Q * (2 * i + 1) / (2 * n);
            const uint32_t w = side || (uint32_t)(2 + i) >= p->n ? 65536u : pb_u16(p, 2u + (uint32_t)i);
            const char* s = pb_str(p, &at);
            char name[16];
            int32_t ty = (y >> 4) - 9;
            str_cpy(name, s, (unsigned)(room + 1 < 16 ? room + 1 : 16));
            pb_cable(side ? x * PB_Q - 1 : 0, y, nx * PB_Q, ny * PB_Q, 64u + (192u * w >> 16));
            ty = ty < 5 ? 5 : ty;
            lx[k] = side ? x - 2 : x + 2, ly[k] = ty, la[k] = 102u + (154u * w >> 16), al[k] = side ? 2 : 0;
            str_cpy(names[k++], name, 16);
        }
    }
    for (i = 0; i < k; i++) {                               /* the names over every cable, each cut out of what it crosses */
        int32_t dx, dy;
        for (dy = -1; dy <= 1; dy++)
            for (dx = -1; dx <= 1; dx++)
                if (dx || dy)
                    pb_label(lx[i] + dx, ly[i] + dy, names[i], pb_bot, 200, al[i]);
        pb_label(lx[i], ly[i], names[i], pb_ink, la[i], al[i]);
    }
    pb_annulus(nx * PB_Q, ny * PB_Q, r * PB_Q, 0, pb_bot, 256);
    pb_annulus(nx * PB_Q, ny * PB_Q, r * PB_Q + 12, r * PB_Q - 12, pb_ink, 256);
    pb_label(nx + 1, ny, MODE[!nin || !nout ? 0 : nin == 1 && nout == 1 ? 1 : nin == 1 ? 2 : 3], pb_ink, 256, 1);
}

/* ---- impulse traces (ImpulseTrace): a baseline with a spike at each echo; times in ms across the window */
#define PB_SPIKES 260
static uint16_t pb_sp_at[2][PB_SPIKES], pb_sp_h[2][PB_SPIKES];   /* ms, height Q15 */
static uint32_t pb_sp_n[2];

static int32_t pb_tx(int32_t ms, int32_t window)            /* 1/16 px */
{
    return 40 + (PB_W * PB_Q - 80) * ms / window;
}

static void pb_impulse(uint32_t side, int32_t window, int32_t top, int32_t h)
{
    const int32_t base = (top + h) * PB_Q;
    uint32_t i, j;
    for (i = 1; i < pb_sp_n[side]; i++)                     /* by time (they come in pass by pass) */
        for (j = i; j > 0 && pb_sp_at[side][j - 1] > pb_sp_at[side][j]; j--) {
            uint16_t t = pb_sp_at[side][j];
            pb_sp_at[side][j] = pb_sp_at[side][j - 1], pb_sp_at[side][j - 1] = t;
            t = pb_sp_h[side][j];
            pb_sp_h[side][j] = pb_sp_h[side][j - 1], pb_sp_h[side][j - 1] = t;
        }
    pb_span_clear();
    pb_px = 0;
    pb_py = base;
    for (i = 0; i < pb_sp_n[side]; i++) {
        const int32_t x = pb_tx(pb_sp_at[side][i], window);
        pb_line_to(x - 40, base);
        pb_line_to(x, base - (int32_t)((uint32_t)h * PB_Q * pb_sp_h[side][i] >> 15));
        pb_line_to(x + 40, base);
    }
    pb_line_to(PB_W * PB_Q - 1, base);
    pb_hatch(top + h, 56);
    pb_trace(256, 1);
}

/* ---- delay (DelayDisplay): the echoes of one tick, left over right, swept in real time */
static uint32_t pb_tanh(uint32_t x)                         /* Q16, x >= 0 */
{
    uint32_t e;
    if (x >= 8u << 16)
        return 65536u;
    e = pb_exp2(-(int32_t)((uint64_t)x * 189097u >> 16));  /* e^-2x */
    e = e ? e : 1u;
    return ((65536u - e) << 16) / (65536u + e);
}

static uint32_t pb_rolloff(uint32_t u)                     /* 1 / sqrt(1 + u^2), Q16 */
{
    const uint32_t s = 65536u + (uint32_t)((uint64_t)u * u >> 16);
    return (0x80000000u / (pb_isqrt(s) << 8)) << 1;
}

static void pb_delay(const fb_pic_t* p)
{
    static int32_t at[4], lvl[4];
    static uint8_t sd[4];
    const int32_t window = 4000, time = pb_v(p, 0) < 1 ? 1 : pb_v(p, 0);
    const uint32_t fb = pb_u16(p, 1), mix = pb_u16(p, 2), stereo = (uint32_t)pb_v(p, 5), freeze = (uint32_t)pb_v(p, 8) & 1u;
    const int32_t lc = pb_v(p, 3) < 1 ? 1 : pb_v(p, 3), hc = pb_v(p, 4) < 1 ? 1 : pb_v(p, 4);
    const uint32_t cl = 6u * pb_exp2(-(int32_t)(4u * pb_u16(p, 7))), inv = 0xFFFFFFFFu / cl;
    const uint32_t tone = pb_rolloff((uint32_t)(1000 * 65536 / hc)) * (uint64_t)pb_rolloff((uint32_t)(lc * 65536 / 1000)) >> 16;
    const int32_t times[2] = { time, (int32_t)((uint32_t)time * (65536u + pb_u16(p, 6) / 2u) >> 16) };
    const int32_t clock = pb_live(p, 0) * window / (int32_t)FB_V_MAX, full = 6, lh = (PB_H - 12 - 8) / 2;
    uint32_t np = 0, count = 0, side, i;
    int32_t sweep;
    for (side = 0; side < 2; side++) {
        pb_sp_n[side] = 1;
        pb_sp_at[side][0] = 0;
        pb_sp_h[side][0] = (uint16_t)((65536u - mix) >> 1);
    }
    for (side = 0; side < (stereo == 1u ? 1u : 2u); side++) {   /* ping-pong: both in on the left */
        const uint32_t x = (uint32_t)((uint64_t)65536u * inv >> 16);
        at[np] = 0, sd[np] = (uint8_t)side, lvl[np++] = (int32_t)(pb_tanh(x) * (uint64_t)cl >> 16);
    }
    while (np) {
        const uint32_t s = sd[--np];
        const int32_t t = at[np] + times[s], l = lvl[np];
        const uint32_t out = (uint32_t)((uint64_t)(uint32_t)l * tone >> 16), heard = (uint32_t)((uint64_t)out * mix >> 16);
        if (count++ >= 256u)
            break;
        if (t >= window || heard <= 655u)
            continue;
        if (pb_sp_n[s] < PB_SPIKES) {
            pb_sp_at[s][pb_sp_n[s]] = (uint16_t)t;
            pb_sp_h[s][pb_sp_n[s]++] = (uint16_t)(heard >= 65536u ? 32767u : heard >> 1);
        }
        at[np] = t;
        sd[np] = (uint8_t)(stereo != 0u ? 1u - s : s);
        if (freeze) {
            lvl[np++] = l;
        } else {
            const uint32_t x = (uint32_t)((uint64_t)((uint64_t)out * fb >> 16) * inv >> 16);
            lvl[np++] = (int32_t)(pb_tanh(x) * (uint64_t)cl >> 16);
        }
    }
    for (side = 0; side < 2; side++) {
        const int32_t top = side ? PB_H - full - lh : full;
        pb_impulse(side, window, top, lh);
        pb_label(PB_W - 6, top + 6, side ? "R" : "L", pb_ink, 102, 1);
    }
    sweep = pb_tx(clock, window) >> 4;
    pb_vdash(sweep, full, PB_H - full, pb_ink, 102);
    for (side = 0; side < 2; side++) {                      /* the dot rides the sweep, up on an echo as it fires */
        const int32_t top = side ? PB_H - full - lh : full;
        int32_t y = top + lh, lit = 0;
        for (i = 0; i < pb_sp_n[side]; i++)
            if (clock >= pb_sp_at[side][i] && clock - pb_sp_at[side][i] <= 150) {
                y = top + lh - (int32_t)((uint32_t)lh * pb_sp_h[side][i] >> 15);
                lit = 1;
                break;
            }
        pb_dot(sweep, y, lit);
    }
}

/* ---- reverb (ReverbDisplay): the dry tick, then each side's tail in dB (as the app's engine renders it) */
static void pb_tail(const fb_pic_t* p, uint32_t from, uint32_t n, uint32_t mix, int32_t h, int32_t base)
{
    int32_t y0 = 0;
    uint32_t i;
    pb_span_clear();
    for (i = 0; i < n; i++) {
        const int32_t x = pb_tx((int32_t)(2u * i + 1u), 2 * (int32_t)n);
        const int32_t y = base - (int32_t)((uint32_t)(h * PB_Q) * (mix >> 4) / 127u * (p->arr[from + i] & 127u) >> 12);
        if (i)
            pb_seg(pb_px, y0, x, y);
        pb_px = x;
        y0 = y;
    }
}

static void pb_reverb(const fb_pic_t* p)
{
    const uint32_t mix = pb_u16(p, 0), n = p->arr_n / 2u > 120u ? 120u : p->arr_n / 2u;
    const int32_t win = pb_v(p, 1) < 1 ? 1 : pb_v(p, 1), top = 6, h = PB_H - 12, base = (top + h) * PB_Q;
    char b[12];
    pb_sp_n[0] = 1;
    pb_sp_at[0][0] = 0;
    pb_sp_h[0][0] = (uint16_t)((65536u - mix) >> 1);
    pb_impulse(0, win * 1000, top, h);
    if (n) {
        pb_tail(p, 0, n, mix, h, base);
        pb_hatch(top + h, 56);
        pb_tail(p, n, n, mix, h, base);
        pb_trace(102, 0);
        pb_tail(p, 0, n, mix, h, base);
        pb_trace(256, 1);
    }
    b[0] = 0;
    pb_cat_int(b, win);
    pb_cat(b, " S");
    pb_label(PB_W, top + 7, b, pb_ink, 102, 2);
}

/* ================================================================ */
static int fb_pics_b_draw(const fb_pic_t* pic, const fb_theme_t* th)
{
    switch (pic->kind) {
    case FB_K_LFO:
    case FB_K_ENVELOPE:
    case FB_K_SEQUENCER:
    case FB_K_ARP:
    case FB_K_CHORD:
    case FB_K_MIDI:
    case FB_K_AUDIO_IN:
    case FB_K_OUTPUT:
    case FB_K_BRIDGE:
    case FB_K_DELAY:
    case FB_K_REVERB:
    case FB_K_MACRO:
        break;
    default:
        return 0;
    }
    pb_begin(th, pic->kind == FB_K_MACRO);
    switch (pic->kind) {
    case FB_K_LFO: pb_lfo(pic); break;
    case FB_K_ENVELOPE: pb_envelope(pic); break;
    case FB_K_SEQUENCER: pb_sequencer(pic); break;
    case FB_K_ARP: pb_arp(pic); break;
    case FB_K_CHORD: pb_chord(pic); break;
    case FB_K_MIDI: pb_midi(pic); break;
    case FB_K_MACRO: pb_macro(pic); break;
    case FB_K_AUDIO_IN: pb_audio_in(pic); break;
    case FB_K_OUTPUT: pb_output(pic); break;
    case FB_K_BRIDGE: pb_bridge(pic); break;
    case FB_K_DELAY: pb_delay(pic); break;
    default: pb_reverb(pic); break;
    }
    cv_blit(FB_PIC_X, FB_PIC_Y);
    return 1;
}

#endif
