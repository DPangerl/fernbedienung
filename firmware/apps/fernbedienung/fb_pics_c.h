/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Daniel Pangerl */
/* Fernbedienung's pictures, part C: the hacker crews' modules (docs/fernbedienung/pictures-c.md; unity build, included
 * once by app.c after part A, whose drawing kit it uses). Each crew draws in its own look, not the display's theme:
 * VÝPIS (ZKRAT) a memory monitor, COMBINÉ (LIGNE) a stencilled poster, MAGE (KAIRO) a neon lens, SCHLIERE (RASTER)
 * a technical drawing. */
#ifndef FB_PICS_C_H
#define FB_PICS_C_H

/* ---- the kit: sine, a filled convex polygon, a ring, glowing text */
static int32_t pc_sin(uint32_t ph) { return FW_SINE[(ph >> 6) & 1023u]; }   /* ph Q16 a cycle, out Q15 */
static int32_t pc_cos(uint32_t ph) { return pc_sin(ph + 16384u); }

/* a convex polygon, Q4 corners in order */
static void pc_poly(const int32_t* px, const int32_t* py, int32_t n, uint16_t c, int32_t a)
{
    int32_t i, r, lo = py[0], hi = py[0];
    for (i = 1; i < n; i++) {
        lo = pa_min(lo, py[i]);
        hi = pa_max(hi, py[i]);
    }
    for (r = pa_max(lo >> 4, 0); r <= pa_min(hi >> 4, PA_H - 1); r++) {
        const int32_t yc = r * 16 + 8;
        int32_t xl = 1 << 20, xr = -(1 << 20);
        for (i = 0; i < n; i++) {
            const int32_t j = i + 1 == n ? 0 : i + 1;
            if ((py[i] <= yc && py[j] > yc) || (py[j] <= yc && py[i] > yc)) {
                const int32_t x = px[i] + (px[j] - px[i]) * (yc - py[i]) / (py[j] - py[i]);
                xl = pa_min(xl, x);
                xr = pa_max(xr, x);
            }
        }
        if (xl < xr)
            pa_hspan(r, xl, xr, c, a);
    }
}

/* a rectangle's outline, 1 px */
static void pc_frame(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c, int32_t a)
{
    pa_rect(x, y, w, 1, c, a);
    pa_rect(x, y + h - 1, w, 1, c, a);
    pa_rect(x, y + 1, 1, h - 2, c, a);
    pa_rect(x + w - 1, y + 1, 1, h - 2, c, a);
}

/* a ring of radius r (px) round cx, cy (px), 1 px */
static void pc_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c, int32_t a)
{
    const int32_t n = 6 * r + 6;
    int32_t i;
    for (i = 0; i < n; i++) {
        const uint32_t ph = (uint32_t)(i * 65536 / n);
        pa_px(cx + ((r * pc_cos(ph) + 16384) >> 15), cy + ((r * pc_sin(ph) + 16384) >> 15), c, a);
    }
}

/* text with a soft halo of colour g round it (neon) */
static int32_t pc_glow_text(int32_t x, int32_t y, const felucca_font_t* f, const char* s, uint16_t c, uint16_t g)
{
    static const int8_t D[8][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }, { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 } };
    int32_t i;
    for (i = 0; i < 8; i++)
        pa_text(x + D[i][0], y + D[i][1], f, s, g, i < 4 ? 90 : 40);
    return pa_text(x, y, f, s, c, 256);
}

/* a hash of n, 0..65535 (spatter, paper grain) */
static uint32_t pc_hash(uint32_t n)
{
    n = (n ^ 61u) ^ (n >> 16);
    n *= 9u;
    n ^= n >> 4;
    n *= 0x27D4EB2Du;
    return (n ^ (n >> 15)) & 0xFFFFu;
}

/* ==== VÝPIS (ZKRAT): v0 address, v1 length (0..4: 16..256 bytes), v2 glitch, v3 level; the array the cycle's bytes,
   each >> 1; live: address, level. The monitor in ZKRAT's memory: the address and the length and misreads in their
   rings' colours, the memory map with the block being read, then the cycle as bits and as the stepped level it plays,
   a read head walking through it once every two seconds */
#define PC_ZK_CYAN RGB(77, 235, 255)
#define PC_ZK_MAGENTA RGB(255, 82, 219)
#define PC_ZK_YELLOW RGB(255, 230, 77)
#define PC_ZK_GREEN RGB(107, 255, 128)
#define PC_ZK_BLUE RGB(92, 117, 255)
#define PC_ZK_WHITE RGB(242, 247, 255)
#define PC_ZK_X 4                           /* the cycle's left edge, px */
#define PC_ZK_W 232
#define PC_ZK_BITS 25                       /* seven bits (the array's), a row of 2 px each */
#define PC_ZK_MID 62                        /* the level's zero */
#define PC_ZK_HALF 19

static void pc_dump(const fb_pic_t* p)
{
    const int32_t addr = pa_live(p, 0) >= 0 ? pa_live(p, 0) : pa_v(p, 0);
    const int32_t offset = pa_min(addr >> 2, 4095) * 16, n = p->arr_n;
    const int32_t step = n ? PC_ZK_W * 16 / n : 0, gap = step > 48 ? 16 : 0;
    const int32_t head = n ? (int32_t)(fm1_ms % 2000u) * n / 2000 : -1;
    int32_t i, k, x;
    cv_begin(FB_PIC_W, FB_PIC_H, RGB(3, 4, 16));

    pc_glow_text(4, 1, &FONT_S, "V\xDDPIS", pa_mix(PC_ZK_CYAN, 0xFFFFu, 128), PC_ZK_CYAN);   /* (the values: the tiles) */
    pa_text(236 - text_w(&FONT_S, "64K"), 1, &FONT_S, "64K", PC_ZK_BLUE, 200);

    for (i = 0; i < 32; i++)                /* the 64 KB in 2 KB blocks, the one being read white */
        for (k = 0; k < 3; k++)
            pa_hspan(19 + k, PC_ZK_X * 16 + i * PC_ZK_W / 2, PC_ZK_X * 16 + (i + 1) * PC_ZK_W / 2 - 24,
                     i == offset / 2048 ? PC_ZK_WHITE : PC_ZK_BLUE, i == offset / 2048 ? 256 : 77);

    pa_hspan(PC_ZK_MID, PC_ZK_X * 16, (PC_ZK_X + PC_ZK_W) * 16, PC_ZK_BLUE, 128);
    for (i = 0; i <= 4; i++)
        pa_vspan(PC_ZK_X + i * (PC_ZK_W - 1) / 4, (PC_ZK_MID - 3) * 16, (PC_ZK_MID + 4) * 16, PC_ZK_BLUE, 128);
    if (!n) {
        cv_blit(FB_PIC_X, FB_PIC_Y);
        return;
    }
    if (head >= 0) {                        /* the read head's column, under the rest */
        const int32_t x0 = (PC_ZK_X * 16 + head * step) / 16 - 1, x1 = (PC_ZK_X * 16 + head * step + step - gap + 15) / 16 + 1;
        pa_rect(x0, PC_ZK_BITS - 2, x1 - x0, PA_H - PC_ZK_BITS + 1, PC_ZK_WHITE, 31);
        pc_frame(x0, PC_ZK_BITS - 2, x1 - x0, PA_H - PC_ZK_BITS + 1, PC_ZK_WHITE, 128);
    }
    for (x = 0; x < PA_W; x++)
        pa_ya[x] = PA_NONE;
    for (i = 0; i < n; i++) {
        const int32_t xq = PC_ZK_X * 16 + i * step, byte = (int32_t)(int8_t)(uint8_t)(p->arr[i] << 1);
        const int32_t yq = PC_ZK_MID * 16 - byte * PC_ZK_HALF * 16 / 128;
        for (k = 0; k < 7; k++)
            pa_hspan(PC_ZK_BITS + 2 * k, xq, xq + step - gap, (p->arr[i] >> (6 - k)) & 1u ? PC_ZK_YELLOW : PC_ZK_BLUE,
                     (p->arr[i] >> (6 - k)) & 1u ? 205 : 36);
        for (x = xq >> 4; x < pa_min((xq + step - gap + 15) >> 4, PA_W); x++)
            pa_vspan(x, yq, PC_ZK_MID * 16, byte >= 0 ? PC_ZK_CYAN : PC_ZK_MAGENTA, 77);
        for (x = xq >> 4; x < pa_min((xq + step + 15) >> 4, PC_ZK_X + PC_ZK_W); x++)
            pa_ya[x] = (int16_t)yq;
    }
    pa_ink = PC_ZK_CYAN;                    /* the stepped trace it plays, glowing along the bars' ends */
    pa_glow(pa_ya, PC_ZK_X, PC_ZK_X + PC_ZK_W);
    for (x = PC_ZK_X + 1; x < PC_ZK_X + PC_ZK_W; x++)
        if (pa_ya[x] != PA_NONE && pa_ya[x - 1] != PA_NONE && pa_ya[x] != pa_ya[x - 1])
            pa_vspan(x, pa_ya[x - 1], pa_ya[x], PC_ZK_WHITE, 256);
    pa_trace(pa_ya, PC_ZK_X, PC_ZK_X + PC_ZK_W, PC_ZK_WHITE, 256, 10, 0);
    cv_blit(FB_PIC_X, FB_PIC_Y);
}

/* ==== COMBINÉ (LIGNE): v0 bands (2..8), v1 crackle, v2 tone; live: each band's level. LIGNE's poster: the shout fanned
   out of a stencilled megaphone, one ray of blocks per band, lowest at the bottom, inked out to the band's level, its
   loudest block red; crackle spatters paint round the rays; the name stencilled at the top, the settings on tape */
#define PC_LG_PAPER RGB(235, 227, 207)
#define PC_LG_INK RGB(18, 15, 18)
#define PC_LG_ROUGE RGB(219, 28, 38)
#define PC_LG_BLEU RGB(18, 61, 179)
#define PC_LG_MX 30                         /* the megaphone's mouth, px */
#define PC_LG_MY 64
#define PC_LG_LOW 1456                      /* the fan: +8 degrees (down) for the lowest band, Q16 a cycle */
#define PC_LG_HIGH (-6554)                  /* -36 degrees (up) for the highest */
#define PC_LG_START 12                      /* the first block's distance from the mouth, px */
#define PC_LG_BLOCK 8
#define PC_LG_GAP 3
#define PC_LG_BLOCKS 18

/* a point at distance r (px) along angle ph from the mouth, Q4; the fan squashed upright to fit the strip */
static void pc_lg_at(int32_t r, int32_t ph, int32_t* x, int32_t* y)
{
    *x = PC_LG_MX * 16 + ((r * 16 * pc_cos((uint32_t)ph)) >> 15);
    *y = PC_LG_MY * 16 + ((r * 12 * pc_sin((uint32_t)ph)) >> 15);
}

static void pc_combine(const fb_pic_t* p)
{
    const int32_t bands = pa_clamp(pa_v(p, 0), 1, 8), spread = (PC_LG_LOW - PC_LG_HIGH) / bands * 34 / 100;
    const int32_t reach = PC_LG_START + PC_LG_BLOCKS * (PC_LG_BLOCK + PC_LG_GAP);
    int32_t band, i, x, y, px[4], py[4];
    cv_begin(FB_PIC_W, FB_PIC_H, PC_LG_PAPER);
    for (y = 0; y < PA_H; y++)              /* the paper's grain */
        for (x = 0; x < PA_W; x++) {
            const uint32_t h = pc_hash((uint32_t)(y * PA_W + x));
            if (h < 9000u)
                pa_px(x, y, RGB(120, 100, 70), (int32_t)(h & 31u));
        }
    px[0] = 0;                              /* a blue slash brushed behind the megaphone */
    py[0] = 84 * 16;
    px[1] = 14 * 16;
    py[1] = 84 * 16;
    px[2] = 70 * 16;
    py[2] = 0;
    px[3] = 56 * 16;
    py[3] = 0;
    pc_poly(px, py, 4, PC_LG_BLEU, 150);

    for (band = 0; band < bands; band++) {
        const int32_t ph = PC_LG_LOW - (PC_LG_LOW - PC_LG_HIGH) * (2 * band + 1) / (2 * bands);
        const int32_t level = pa_live(p, (uint32_t)band) > 0 ? pa_live(p, (uint32_t)band) : 0;
        const int32_t inked = (level * PC_LG_BLOCKS + 8192) >> 14;
        for (i = 0; i < PC_LG_BLOCKS; i++) {
            const int32_t r0 = PC_LG_START + i * (PC_LG_BLOCK + PC_LG_GAP), r1 = r0 + PC_LG_BLOCK;
            pc_lg_at(r0, ph - spread, &px[0], &py[0]);
            pc_lg_at(r0, ph + spread, &px[1], &py[1]);
            pc_lg_at(r1, ph + spread, &px[2], &py[2]);
            pc_lg_at(r1, ph - spread, &px[3], &py[3]);
            if (py[1] < 0)                  /* off the sheet's top */
                break;
            if (i >= inked)
                pc_poly(px, py, 4, PC_LG_INK, 22);
            else
                pc_poly(px, py, 4, i == inked - 1 ? PC_LG_ROUGE : PC_LG_INK, 256);
        }
    }
    for (i = 0; i < (pa_v(p, 1) * 70 >> 14); i++) {   /* crackle's spatter */
        const int32_t ph = PC_LG_HIGH + (int32_t)(pc_hash((uint32_t)i) * (uint32_t)(PC_LG_LOW - PC_LG_HIGH) >> 16);
        const int32_t r = PC_LG_START + (int32_t)(pc_hash((uint32_t)i + 500u) * (uint32_t)(reach - PC_LG_START) >> 16);
        const int32_t d = 1 + (int32_t)(pc_hash((uint32_t)i + 900u) >> 15);
        pc_lg_at(r, ph, &x, &y);
        pa_rect(x >> 4, y >> 4, d, d, PC_LG_INK, 180);
    }

    px[0] = 4 * 16;                         /* the megaphone, a stencil */
    py[0] = (PC_LG_MY - 5) * 16;
    px[1] = PC_LG_MX * 16;
    py[1] = (PC_LG_MY - 12) * 16;
    px[2] = PC_LG_MX * 16;
    py[2] = (PC_LG_MY + 12) * 16;
    px[3] = 4 * 16;
    py[3] = (PC_LG_MY + 5) * 16;
    pc_poly(px, py, 4, PC_LG_INK, 256);
    pa_rect(PC_LG_MX - 3, PC_LG_MY - 12, 2, 24, PC_LG_PAPER, 256);   /* the stencil's bridge */
    pa_rect(10, PC_LG_MY + 5, 4, 10, PC_LG_INK, 256);                /* the grip */
    pa_rect(1, PC_LG_MY - 2, 3, 4, PC_LG_ROUGE, 256);

    x = 234 - text_w(&FONT_L, "COMBINE");   /* the name stencilled across the top, its accent cut by hand */
    pa_text(x, 0, &FONT_L, "COMBINE", PC_LG_INK, 256);
    px[0] = (x + 100) * 16;
    py[0] = 6 * 16;
    px[1] = (x + 105) * 16;
    py[1] = 1 * 16;
    px[2] = (x + 109) * 16;
    py[2] = 1 * 16;
    px[3] = (x + 103) * 16;
    py[3] = 6 * 16;
    pc_poly(px, py, 4, PC_LG_ROUGE, 256);

    cv_blit(FB_PIC_X, FB_PIC_Y);
}

/* ==== MAGE (KAIRO): v0 shape (0 saw, 1 square, 2 pulse, 3 reso), v1 bend, v2 level; live: bend, level. KAIRO's
   lens: in it the note's cycle as it sounds, in pink tube over the plain cosine it was bent from, in cyan; sixty-four
   ticks round its rim. Left the name and the bent phase itself, right the shape and how far it is bent */
#define PC_KA_PINK RGB(255, 69, 163)
#define PC_KA_CYAN RGB(77, 240, 255)
#define PC_KA_TUBE RGB(255, 217, 242)
#define PC_KA_CX 120
#define PC_KA_CY 42
#define PC_KA_R 38

/* Engine/src/Mage.cpp's bend: where in the cosine phase p reads (Q16 both), bend Q14 */
static uint32_t pc_ka_bent(uint32_t p, int32_t shape, int32_t bend)
{
    if (shape == 0) {
        const uint32_t knee = 32768u - (uint32_t)(32113 * bend >> 14);
        return p < knee ? p * 32768u / knee : 32768u + (p - knee) * 32768u / (65536u - knee);
    }
    if (shape == 1) {
        const uint32_t run = 32768u - (uint32_t)(31457 * bend >> 14);
        return p < 32768u ? (p < run ? p : run) * 32768u / run
                          : 32768u + (p - 32768u < run ? p - 32768u : run) * 32768u / run;
    }
    {
        const uint32_t d = 65536u - (uint32_t)(62259 * bend >> 14), q = (p << 8) / (d >> 8);
        return q < 65536u ? q : 65536u;
    }
}

static int32_t pc_ka_sample(uint32_t p, int32_t shape, int32_t bend)   /* Q15 */
{
    if (shape == 3)
        return pc_sin(p + (uint32_t)(((int32_t)(p * 15u >> 7) * bend) >> 7)) * (int32_t)(65536u - p) >> 16;
    return -pc_cos(pc_ka_bent(p, shape, bend));
}

static void pc_mage(const fb_pic_t* p)
{
    const int32_t shape = pa_min(pa_v(p, 0), 3), bend = pa_live(p, 0) >= 0 ? pa_live(p, 0) : pa_v(p, 1);
    const int32_t x0 = PC_KA_CX - PC_KA_R * 86 / 100, x1 = PC_KA_CX + PC_KA_R * 86 / 100, half = PC_KA_R / 2;
    int32_t i, x;
    cv_begin(FB_PIC_W, FB_PIC_H, RGB(5, 4, 16));
    for (i = 0; i < 64; i++) {              /* the compass's ticks round the lens */
        const uint32_t ph = (uint32_t)i * 1024u;
        const int32_t r0 = PC_KA_R + 2, r1 = PC_KA_R + (i % 8 ? 3 : 5);
        pa_line(PC_KA_CX * 16 + ((r0 * 16 * pc_cos(ph)) >> 15), PC_KA_CY * 16 + ((r0 * 16 * pc_sin(ph)) >> 15),
                PC_KA_CX * 16 + ((r1 * 16 * pc_cos(ph)) >> 15), PC_KA_CY * 16 + ((r1 * 16 * pc_sin(ph)) >> 15),
                i % 8 ? PC_KA_CYAN : PC_KA_PINK, i % 8 ? 70 : 200, 0);
    }
    pc_ring(PC_KA_CX, PC_KA_CY, PC_KA_R, PC_KA_CYAN, 110);
    pc_ring(PC_KA_CX, PC_KA_CY, PC_KA_R - 4, PC_KA_CYAN, 34);
    pa_hspan(PC_KA_CY, x0 * 16, x1 * 16, PC_KA_CYAN, 56);
    for (i = 0; i <= 4; i++)
        pa_vspan(x0 + (x1 - x0) * i / 4, (PC_KA_CY - 3) * 16, (PC_KA_CY + 4) * 16, PC_KA_CYAN, 56);

    for (x = 0; x < PA_W; x++)
        pa_ya[x] = pa_yb[x] = PA_NONE;
    for (x = x0; x <= x1; x++) {
        const uint32_t ph = (uint32_t)((x - x0) * 65536 / (x1 - x0));
        pa_yb[x] = (int16_t)(PC_KA_CY * 16 + (pc_cos(ph) * half * 16 >> 15));
        pa_ya[x] = (int16_t)(PC_KA_CY * 16 - (pc_ka_sample(ph, shape, bend) * half * 16 >> 15));
    }
    pa_trace(pa_yb, x0, x1 + 1, PC_KA_CYAN, 115, 8, 0);
    pa_ink = PC_KA_PINK;
    pa_glow(pa_ya, x0, x1 + 1);
    pa_trace(pa_ya, x0, x1 + 1, PC_KA_TUBE, 256, 12, 0);

    pc_glow_text(40 - text_w(&FONT_M, "MAGE") / 2, 32, &FONT_M, "MAGE", PC_KA_TUBE, PC_KA_PINK);
    pc_frame(182, 24, 36, 36, PC_KA_CYAN, 77);   /* the bent phase against the straight (the values: the tiles) */
    for (x = 0; x < PA_W; x++)
        pa_yb[x] = PA_NONE;
    for (x = 183; x < 217; x++) {
        const uint32_t ph = (uint32_t)((x - 183) * 65536 / 33), q = shape == 3 ? ph : pc_ka_bent(ph, shape, bend);
        pa_yb[x] = (int16_t)(58 * 16 - (int32_t)(q * 34u * 16u >> 16));
    }
    pa_trace(pa_yb, 183, 217, PC_KA_CYAN, 256, 8, 0);
    pa_text(200 - text_w(&FONT_T, "PHASE") / 2, 64, &FONT_T, "PHASE", PC_KA_CYAN, 170);
    cv_blit(FB_PIC_X, FB_PIC_Y);
}

/* ==== SCHLIERE (RASTER): v0 glide (log, 0.01..4 s), v1 saws (1..7), v2 spread, v3 level. RASTER's drawing, view A:
   each saw's pitch sliding from the last note up to the new one at its own pace, the fastest in the crew's gold;
   the glide time dimensioned on the time axis, the spread between the saws' targets at the right */
#define PC_RA_GOLD RGB(242, 217, 102)
#define PC_RA_LINE 153                      /* the drawing's white, /256 */
#define PC_RA_FAINT 87
#define PC_RA_X0 30                         /* the plot, px */
#define PC_RA_X1 180
#define PC_RA_Y0 26
#define PC_RA_Y1 56

/* where a saw stands (reached Q16: 0 the last note, 1 the new one) as a height, Q4 */
static int32_t pc_ra_y(int32_t reached)
{
    return PC_RA_Y1 * 16 - ((PC_RA_Y1 - PC_RA_Y0) * 16 * (9830 + (45875 * (reached >> 4) >> 12)) >> 16);
}

/* a closed arrowhead at tip (Q4), pointing along (dx, dy) one of the four ways */
static void pc_ra_arrow(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t a)
{
    int32_t px[3], py[3];
    px[0] = x;
    py[0] = y;
    px[1] = x - dx * 80 - dy * 22;
    py[1] = y - dy * 80 - dx * 22;
    px[2] = x - dx * 80 + dy * 22;
    py[2] = y - dy * 80 + dx * 22;
    pc_poly(px, py, 3, 0xFFFFu, a);
}

/* a dimension between two points on one line (Q4), arrowheads in */
static void pc_ra_dim(int32_t xa, int32_t ya, int32_t xb, int32_t yb)
{
    const int32_t dx = xb > xa ? 1 : xb < xa ? -1 : 0, dy = yb > ya ? 1 : yb < ya ? -1 : 0;
    pa_line(xa, ya, xb, yb, 0xFFFFu, PC_RA_LINE, 0);
    pc_ra_arrow(xa, ya, -dx, -dy, PC_RA_LINE);
    pc_ra_arrow(xb, yb, dx, dy, PC_RA_LINE);
}

static void pc_smear(const fb_pic_t* p)
{
    const int32_t count = pa_clamp(pa_v(p, 1), 1, 7), spread = pa_v(p, 2);
    const int32_t glide = PC_RA_X0 + (PC_RA_X1 - PC_RA_X0) / 3, w = PC_RA_X1 - PC_RA_X0;
    int32_t i, v, x, y;
    cv_begin(FB_PIC_W, FB_PIC_H, RGB(14, 16, 20));
    pa_text(4, 1, &FONT_S, "ANSICHT A", 0xFFFFu, PC_RA_LINE);
    pa_text(88, 1, &FONT_S, "Tonh\xF6he \xFC" "ber Zeit", 0xFFFFu, PC_RA_FAINT);

    pa_vspan(PC_RA_X0, (PC_RA_Y0 - 6) * 16, (PC_RA_Y1 + 22) * 16, 0xFFFFu, PC_RA_LINE);   /* the axes */
    pa_hspan(PC_RA_Y1, PC_RA_X0 * 16, (PC_RA_X1 + 10) * 16, 0xFFFFu, PC_RA_LINE);
    pc_ra_arrow(PC_RA_X0 * 16 + 8, (PC_RA_Y0 - 9) * 16, 0, -1, PC_RA_LINE);
    pc_ra_arrow((PC_RA_X1 + 14) * 16, PC_RA_Y1 * 16 + 8, 1, 0, PC_RA_LINE);
    pa_text(PC_RA_X0 - 10, PC_RA_Y0 - 14, &FONT_T, "f", 0xFFFFu, PC_RA_LINE);
    pa_text(PC_RA_X1 + 10, PC_RA_Y1 - 15, &FONT_T, "t", 0xFFFFu, PC_RA_LINE);
    for (i = 1; i <= 3; i++) {
        static const char* const T[3] = { "T", "2T", "3T" };
        x = PC_RA_X0 + w * i / 3;
        pa_vspan(x, PC_RA_Y1 * 16, (PC_RA_Y1 + 3) * 16, 0xFFFFu, PC_RA_LINE);
        pa_text(x - text_w(&FONT_T, T[i - 1]) / 2, PC_RA_Y1 + 4, &FONT_T, T[i - 1], 0xFFFFu, PC_RA_FAINT);
    }
    for (i = 0; i < 2; i++) {               /* the old note and the new one */
        y = pc_ra_y(i ? 65536 : 0) >> 4;
        pa_hdash(y, PC_RA_X0, PC_RA_X1, 0xFFFFu, PC_RA_FAINT);
        pa_text(PC_RA_X0 - 4 - text_w(&FONT_T, i ? "neu" : "alt"), y - 7, &FONT_T, i ? "neu" : "alt", 0xFFFFu, PC_RA_FAINT);
    }
    pa_vspan(glide, (PC_RA_Y1 + 16) * 16, (PC_RA_Y1 + 27) * 16, 0xFFFFu, PC_RA_LINE);   /* the glide, dimensioned */
    pc_ra_dim(PC_RA_X0 * 16 + 8, (PC_RA_Y1 + 24) * 16 + 8, glide * 16 + 8, (PC_RA_Y1 + 24) * 16 + 8);

    for (v = count - 1; v >= 0; v--) {      /* the saws, the fastest (gold) last, on top */
        const int32_t pace = count > 1 ? 64 + 384 * v / (count - 1) : 256;   /* Q8 glides */
        const int32_t detune = count > 1 ? ((v * 131072 / (count - 1) - 65536) * spread >> 14) * 3932 >> 16 : 0;   /* 6 %: Q16 */
        for (x = 0; x < PA_W; x++)
            pa_ya[x] = PA_NONE;
        for (x = PC_RA_X0; x <= PC_RA_X1; x++) {
            const int32_t tq = (x - PC_RA_X0) * 3 * 4096 / w;                  /* glides, Q12 */
            const int32_t l = -(tq * 256 / pace * 5909 >> 12);                    /* -t / pace in log2, Q12 */
            pa_ya[x] = (int16_t)pc_ra_y(65536 - (int32_t)pa_exp2(pa_max(l, -30 * 4096)) + detune);
        }
        pa_trace(pa_ya, PC_RA_X0, PC_RA_X1 + 1, v ? 0xFFFFu : PC_RA_GOLD, v ? 180 : 256, v ? 6 : 10, 0);
        y = pc_ra_y(65536 + detune);
        pa_hspan(y >> 4, (PC_RA_X1 + 3) * 16, (PC_RA_X1 + 22) * 16, 0xFFFFu, PC_RA_FAINT);
    }
    if (count > 1 && spread > 0) {          /* the spread between the targets */
        const int32_t s = spread * 3932 >> 14;
        pc_ra_dim((PC_RA_X1 + 19) * 16, pc_ra_y(65536 + s) - 16, (PC_RA_X1 + 19) * 16, pc_ra_y(65536 - s) + 16);
        pa_text(PC_RA_X1 + 25, (pc_ra_y(65536) >> 4) - 7, &FONT_T, "s", 0xFFFFu, PC_RA_LINE);
    }
    pc_frame(204, 66, 34, 17, 0xFFFFu, PC_RA_FAINT);   /* the title block's scale field */
    pa_text(221 - text_w(&FONT_T, "1:1") / 2, 68, &FONT_T, "1:1", PC_RA_GOLD, 256);
    cv_blit(FB_PIC_X, FB_PIC_Y);
}

/* ---- the crew's screen: the header, tiles and footer in the crew's look (fb_screen.h, FB_CREW_THEMES) */
static uint32_t fb_crew;                    /* 0 none; 1 ZKRAT, 2 LIGNE, 3 KAIRO, 4 RASTER */

static uint32_t pc_crew_of(uint32_t kind)
{
    return kind >= FB_K_DUMP && kind <= FB_K_SMEAR ? kind - FB_K_DUMP + 1u : 0u;
}

static const char* pc_crew_name(void)
{
    static const char* const N[4] = { "ZKRAT", "LIGNE", "KAIRO", "RASTER" };
    return N[(fb_crew - 1u) & 3u];
}

static uint16_t pc_crew_accent(void)
{
    static const uint16_t A[4] = { PC_ZK_CYAN, PC_LG_ROUGE, PC_KA_PINK, PC_RA_GOLD };
    return A[(fb_crew - 1u) & 3u];
}

/* the readout in M where it fits w, else T; returns the font */
static const felucca_font_t* pc_fit(const char* s, int32_t w)
{
    return text_w(&FONT_M, s) <= w ? &FONT_M : &FONT_T;
}

/* ZKRAT: the name inverse on the encoder's colour, the value large, a row of cells lit to it (white: a cable) */
static void pc_tile_zkrat(uint32_t i, const fb_ctl_t* c)
{
    static const uint16_t COL[4] = { PC_ZK_CYAN, PC_ZK_MAGENTA, PC_ZK_GREEN, PC_ZK_YELLOW };
    const uint16_t col = COL[i & 3u];
    const int32_t lit = ((int32_t)c->pos * 16 + 8192) >> 14, live = ((int32_t)c->live * 15 + 8192) >> 14;
    const felucca_font_t* f = pc_fit(c->readout, 104);
    int32_t k;
    pa_rect(6, 4, text_w(&FONT_T, c->title) + 8, 15, col, 256);
    pa_text(10, 5, &FONT_T, c->title, RGB(3, 4, 16), 256);
    pc_glow_text(8, f == &FONT_M ? 21 : 24, f, c->readout, PC_ZK_WHITE, col);
    for (k = 0; k < 16; k++) {
        const int32_t x = 8 + k * 102 / 16;
        pa_rect(x, 43, 5, 5, k < lit ? col : PC_ZK_BLUE, k < lit ? 256 : 60);
        if (c->flags & FB_CTL_LIVE && k == live)
            pc_frame(x - 1, 42, 7, 7, PC_ZK_WHITE, 256);
    }
}

/* LIGNE: the name on black tape, the value stencilled (red near the top), the sweep as stencil blocks */
static void pc_tile_ligne(const fb_ctl_t* c)
{
    const int32_t lit = ((int32_t)c->pos * 12 + 8192) >> 14;
    const felucca_font_t* f = pc_fit(c->readout, 104);
    int32_t k;
    pa_rect(4, 3, text_w(&FONT_T, c->title) + 10, 16, RGB(20, 20, 20), 240);
    pa_text(9, 4, &FONT_T, c->title, PC_LG_PAPER, 256);
    pa_text(8, f == &FONT_M ? 21 : 24, f, c->readout, c->pos > 13107u ? PC_LG_ROUGE : PC_LG_INK, 256);
    for (k = 0; k < 12; k++)
        pa_rect(8 + k * 9, 42, 7, 6, k == lit - 1 ? PC_LG_ROUGE : PC_LG_INK, k < lit ? 256 : 24);
    if (c->flags & FB_CTL_LIVE)
        pa_rect(8 + ((int32_t)c->live * 104 >> 14), 39, 2, 12, PC_LG_BLEU, 256);
}

/* KAIRO: a neon ring per encoder in its guardian's colour, twelve segments lit to the value (gold: a cable),
   the name and the value beside it */
static void pc_tile_kairo(uint32_t i, const fb_ctl_t* c)
{
    static const uint16_t COL[4] = { RGB(56, 158, 255), RGB(255, 92, 41), RGB(158, 97, 255), RGB(240, 240, 255) };
    const uint16_t col = COL[i & 3u];
    const int32_t lit = ((int32_t)c->pos * 12 + 8192) >> 14;
    const felucca_font_t* f = pc_fit(c->readout, 66);
    int32_t k, d, r;
    pc_ring(26, 26, 23, col, 40);
    for (k = 0; k < 12; k++)                /* 300 degrees from the lower left round over the top */
        for (d = 0; d < 21; d++) {
            const uint32_t ph = (uint32_t)((120 + k * 25 + d) * 65536 / 360);
            for (r = 18; r <= 21; r++)
                pa_px(26 + ((r * pc_cos(ph) + 16384) >> 15), 26 + ((r * pc_sin(ph) + 16384) >> 15), col,
                      k < lit ? (r == 18 || r == 21 ? 150 : 256) : 34);
        }
    if (c->flags & FB_CTL_LIVE) {
        const uint32_t ph = (uint32_t)((120 * 16384 + (int32_t)c->live * 300) / 16384 * 65536 / 360);
        pa_rect(26 + ((14 * pc_cos(ph)) >> 15) - 1, 26 + ((14 * pc_sin(ph)) >> 15) - 1, 3, 3, PC_RA_GOLD, 256);
    }
    pa_text(52, 5, &FONT_T, c->title, col, 256);
    pc_glow_text(52, f == &FONT_M ? 21 : 24, f, c->readout, 0xFFFFu, col);
}

/* RASTER: a part on the sheet, framed, its name in a title cell, the value entered as the measure and dimensioned
   from the sweep's start (gold: where a cable has it) */
static void pc_tile_raster(const fb_ctl_t* c)
{
    const int32_t x = 8 + ((int32_t)c->pos * 102 >> 14);
    const felucca_font_t* f = pc_fit(c->readout, 104);
    pc_frame(1, 1, (int32_t)FB_TILE_W - 2, (int32_t)FB_TILE_H - 2, 0xFFFFu, PC_RA_LINE);
    pa_rect(2, 18, (int32_t)FB_TILE_W - 4, 1, 0xFFFFu, PC_RA_FAINT);
    pa_text(6, 3, &FONT_T, c->title, 0xFFFFu, PC_RA_FAINT);
    pa_text(8, f == &FONT_M ? 20 : 23, f, c->readout, 0xFFFFu, 240);
    pa_rect(8, 40, 1, 9, 0xFFFFu, PC_RA_LINE);
    pa_rect(x, 40, 1, 9, 0xFFFFu, PC_RA_LINE);
    if (x - 8 >= 12)
        pc_ra_dim(8 * 16 + 8, 45 * 16, x * 16 + 8, 45 * 16);
    if (c->flags & FB_CTL_LIVE)
        pa_vdash(8 + ((int32_t)c->live * 102 >> 14), 38, 50, PC_RA_GOLD, 256);
}

/* a tile on the canvas fb_tile_draw set up (the crew's ground) */
static void pc_tile(uint32_t i, const fb_ctl_t* c)
{
    switch (fb_crew) {
    case 1: pc_tile_zkrat(i, c); break;
    case 2: pc_tile_ligne(c); break;
    case 3: pc_tile_kairo(i, c); break;
    default: pc_tile_raster(c); break;
    }
}

/* ---- the drawer: 1 if pic is one of part C's kinds (drawn and blitted), else 0 */
static int fb_pics_c_draw(const fb_pic_t* pic)
{
    switch (pic->kind) {
    case FB_K_DUMP: pc_dump(pic); return 1;
    case FB_K_COMBINE: pc_combine(pic); return 1;
    case FB_K_MAGE: pc_mage(pic); return 1;
    case FB_K_SMEAR: pc_smear(pic); return 1;
    default: return 0;
    }
}

/* 1 while pic moves by itself (VÝPIS's read head) */
static int fb_pics_c_moves(const fb_pic_t* pic) { return pic->kind == FB_K_DUMP; }

#endif
