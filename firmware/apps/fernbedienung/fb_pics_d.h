/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Daniel Pangerl */
/* Fernbedienung's pictures, part D: the crews' later modules (docs/fernbedienung/pictures-d.md; unity build, included
 * once by app.c after part C, whose crew looks and kit it uses). BUBEN (ZKRAT) the eight pads, PRISE (LIGNE) a Minitel
 * page with a tuner, KODAMA (KAIRO) the tape loop in gold, MITSCHNITT and EMPFANG (RASTER) on the drawing sheet. */
#ifndef FB_PICS_D_H
#define FB_PICS_D_H

/* a word in fg on a bar of bg, f's height; returns its width */
static int32_t pd_chip(int32_t x, int32_t y, const felucca_font_t* f, const char* s, uint16_t bg, uint16_t fg)
{
    const int32_t w = text_w(f, s) + 8;
    pa_rect(x, y, w, (int32_t)f->h, bg, 256);
    pa_text(x + 4, y, f, s, fg, 256);
    return w;
}

/* ms as seconds with a decimal comma (RASTER's German), one or two digits: "2,35 s" */
static void pd_secs(char* b, int32_t ms, int32_t digits)
{
    const int32_t unit = digits > 1 ? 10 : 100, n = (pa_max(ms, 0) + unit / 2) / unit;
    char d[3];
    b[0] = 0;
    ui_cat_int(b, n / (digits > 1 ? 100 : 10), 16);
    ui_cat(b, ",", 16);
    d[0] = (char)('0' + (digits > 1 ? n / 10 % 10 : n % 10));
    d[1] = digits > 1 ? (char)('0' + n % 10) : 0;
    d[2] = 0;
    ui_cat(b, d, 16);
    ui_cat(b, " s", 16);
}

/* decibels x10 of a frac level (Q14); -999 for silence */
static int32_t pd_db10(int32_t level)
{
    return level > 0 ? (pa_log2((uint32_t)level) - pa_log2(16383u)) * 602 / 40960 : -999;
}

/* ==== BUBEN (ZKRAT): v0 kit (0..3), v1 the sound the knobs turn (0..7), v2 volume, v3 drive, v4 feel (-1..1);
   arr a byte per sound (bits 0-1 choke group, bit 2 turned away from the kit); live: each sound's ring level.
   The kit on the top line with its drive and feel in VÝPIS's pixel font, the eight pads in their lanes' colours, each lit as its sound
   rings, with the key it sits on (Czech note names), a dot where it is edited and its choke group; the pad the knobs
   turn bracketed, and named under the pads */
static const uint16_t PD_BU_LANE[8] = {
    RGB(255, 69, 56), RGB(255, 140, 51), RGB(255, 217, 77), RGB(191, 255, 89),
    RGB(89, 255, 166), RGB(77, 217, 255), RGB(115, 140, 255), RGB(217, 115, 255),
};
static const char* const PD_BU_KIT[4] = { "KOTEL", "PLECH", "SKLEP", "DRAT" };
static const char* const PD_BU_NAME[8] = { "KOPAK", "VIRBL", "HAJTKA Z", "HAJTKA O", "TLESK", "TOM 1", "TOM 2", "PERKUSE" };
static const char* const PD_BU_SOUND[8] = { "KICK", "SNARE", "CL HAT", "OP HAT", "CLAP", "LO TOM", "HI TOM", "PERC" };
static const char* const PD_BU_KEY[8] = { "C", "D", "E", "F", "G", "A", "H", "#" };
#define PD_BU_PAD_Y 20
#define PD_BU_PAD_W 26
#define PD_BU_PAD_H 34
#define PD_BU_PITCH 30                      /* pad to pad */

/* the corners of a bracket round x, y, w, h */
static void pd_brackets(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    const int32_t arm = 6;
    pa_rect(x, y, arm, 2, c, 256);
    pa_rect(x, y, 2, arm, c, 256);
    pa_rect(x + w - arm, y, arm, 2, c, 256);
    pa_rect(x + w - 2, y, 2, arm, c, 256);
    pa_rect(x, y + h - 2, arm, 2, c, 256);
    pa_rect(x, y + h - arm, 2, arm, c, 256);
    pa_rect(x + w - arm, y + h - 2, arm, 2, c, 256);
    pa_rect(x + w - 2, y + h - arm, 2, arm, c, 256);
}

static void pd_buben(const fb_pic_t* p)
{
    const int32_t kit = pa_min(pa_v(p, 0), 3), focus = pa_min(pa_v(p, 1), 7);
    const int32_t feel = (pa_v(p, 4) * 2000 + 8191) / 16383 - 1000;   /* per mille: -1000 loose, 1000 swung */
    const uint16_t fc = PD_BU_LANE[focus];
    char b[24];
    int32_t i, k, x, w, px[3], py[3];
    cv_begin(FB_PIC_W, FB_PIC_H, RGB(3, 4, 16));

    pc_glow_text(4, 1, &FONT_S, PD_BU_KIT[kit], pa_mix(PD_BU_LANE[0], 0xFFFFu, 115), PD_BU_LANE[0]);
    b[0] = 0;
    if (feel > -5 && feel < 5) {
        ui_cat(b, "STRAIGHT", sizeof b);
    } else if (feel > 0) {
        ui_cat(b, "SWING ", sizeof b);
        ui_cat_int(b, 50 + (feel + 20) / 40, sizeof b);
        ui_cat(b, "%", sizeof b);
    } else {
        ui_cat(b, "LOOSE ", sizeof b);
        ui_cat_int(b, (5 - feel) / 10, sizeof b);
    }
    x = 236 - text_w(&FONT_S, b);
    pa_text(x, 1, &FONT_S, b, PD_BU_LANE[2], 256);
    b[0] = 0;
    ui_cat(b, "BUZENI ", sizeof b);
    ui_cat_int(b, (pa_v(p, 3) * 100 + 8191) / 16383, sizeof b);
    ui_cat(b, "%", sizeof b);
    pa_text(x - 16 - text_w(&FONT_S, b), 1, &FONT_S, b, PD_BU_LANE[1], 256);

    for (i = 0; i < 8; i++) {               /* the pads, kick first */
        const uint16_t c = PD_BU_LANE[i];
        const int32_t level = pa_max(pa_live(p, (uint32_t)i), 0), x0 = 2 + i * PD_BU_PITCH;
        const uint32_t flags = (uint32_t)i < p->arr_n ? p->arr[i] : 0u;
        const uint16_t ink = level > 8192 ? RGB(3, 4, 16) : pa_mix(c, 0xFFFFu, 90);
        if (level > 300)
            pa_rect(x0 - 2, PD_BU_PAD_Y - 2, PD_BU_PAD_W + 4, PD_BU_PAD_H + 4, c, level * 90 >> 14);
        pa_rect(x0, PD_BU_PAD_Y, PD_BU_PAD_W, PD_BU_PAD_H, c, 26 + (level * 230 >> 14));
        pc_frame(x0, PD_BU_PAD_Y, PD_BU_PAD_W, PD_BU_PAD_H, c, i == focus ? 256 : 90);
        pa_text(x0 + (PD_BU_PAD_W - text_w(&FONT_M, PD_BU_KEY[i])) / 2, PD_BU_PAD_Y + 4, &FONT_M, PD_BU_KEY[i], ink, 256);
        if (flags & 4u)                     /* turned away from the kit */
            pa_rect(x0 + PD_BU_PAD_W - 6, PD_BU_PAD_Y + 3, 3, 3, ink, 256);
        for (k = 0; k < (int32_t)(flags & 3u); k++)   /* its choke group: that many marks */
            pa_rect(x0 + 4 + k * 5, PD_BU_PAD_Y + PD_BU_PAD_H - 6, 3, 3, ink, 200);
        if (i == focus)
            pd_brackets(x0 - 2, PD_BU_PAD_Y - 2, PD_BU_PAD_W + 4, PD_BU_PAD_H + 4, 0xFFFFu);
    }

    px[0] = 4 * 16;                         /* the sound the knobs turn: a pointer, its name, the English one */
    py[0] = 62 * 16;
    px[1] = 4 * 16;
    py[1] = 76 * 16;
    px[2] = 12 * 16;
    py[2] = 69 * 16;
    pc_poly(px, py, 3, fc, 256);
    pc_glow_text(18, 59, &FONT_M, PD_BU_NAME[focus], pa_mix(fc, 0xFFFFu, 77), fc);
    w = text_w(&FONT_T, PD_BU_SOUND[focus]);
    pa_text(236 - w, 62, &FONT_T, PD_BU_SOUND[focus], fc, 170);
    cv_blit(FB_PIC_X, FB_PIC_Y);
}

/* ==== PRISE (LIGNE): v0 mode (0 SUIVRE, 1 ACCORD), v4 scale (0..3), v5 key (0..11), v11 voices, the rest for the
   tiles; live: note sent + 1 (0 none), pitch heard (hundredths of a semitone from MIDI's 0, 0 silence), input level,
   4 voices' pitch (as heard), 4 voices' level. A Minitel page as Steckfeld's: the mode in inverse and the scale or the
   choir on the top line, the input's level under it; the note heard in big solfège and a tuner beside it that says
   JUSTE, or FAUX in red; the note it sends; the four voices along the foot, each lit as loud as it sings */
#define PD_PR_BLACK RGB(6, 6, 10)
#define PD_PR_BLANC RGB(242, 242, 242)
#define PD_PR_ROUGE RGB(255, 46, 41)
#define PD_PR_JAUNE RGB(255, 235, 51)
#define PD_PR_CYAN RGB(64, 242, 255)
#define PD_PR_CELL 5                        /* the tuner's cell pitch */
#define PD_PR_TX (236 - 21 * PD_PR_CELL)    /* the tuner's left edge */
static const char* const PD_PR_KEY[12] = { "DO", "DO#", "RE", "RE#", "MI", "FA", "FA#", "SOL", "SOL#", "LA", "LA#", "SI" };
static const char* const PD_PR_SCALE[4] = { "CHROMA", "MAJEUR", "MINEUR", "PENTA" };
static const uint16_t PD_PR_MASK[4] = { 0xFFFu, 0xAB5u, 0x5ADu, 0x295u };   /* the scale's notes from the key, C first */
static const uint16_t PD_PR_VOICE[4] = { RGB(51, 102, 255), PD_PR_BLANC, PD_PR_ROUGE, RGB(255, 184, 26) };

static void pd_pr_name(char* b, int32_t note)   /* solfège, French octaves: DO3 is MIDI's 60 */
{
    note = pa_clamp(note, 0, 127);
    b[0] = 0;
    ui_cat(b, PD_PR_KEY[note % 12], 8);
    ui_cat_int(b, note / 12 - 2, 8);
}

/* hundredths of a semitone from the nearest note of the scale (chromatic: the nearest semitone) */
static int32_t pd_pr_off(int32_t heard, int32_t scale, int32_t key)
{
    const int32_t near = (heard + 50) / 100;
    int32_t best = 100000, n;
    for (n = near - 6; n <= near + 6; n++)
        if ((PD_PR_MASK[scale] >> ((n - key + 1200) % 12)) & 1u && pa_abs(heard - n * 100) < pa_abs(best))
            best = heard - n * 100;
    return best;
}

static void pd_prise(const fb_pic_t* p)
{
    const int32_t accord = pa_v(p, 0) == 1, scale = pa_min(pa_v(p, 4), 3), key = pa_min(pa_v(p, 5), 11);
    const int32_t sent = pa_live(p, 0), heard = pa_live(p, 1), level = pa_max(pa_live(p, 2), 0);
    char b[24];
    int32_t i, x, w;
    cv_begin(FB_PIC_W, FB_PIC_H, PD_PR_BLACK);

    pd_chip(0, 0, &FONT_T, accord ? "ACCORD" : "SUIVRE", PD_PR_CYAN, PD_PR_BLACK);
    b[0] = 0;
    if (accord) {
        ui_cat_int(b, pa_clamp(pa_v(p, 11), 1, 4), sizeof b);
        ui_cat(b, " VOIX", sizeof b);
    } else {
        ui_cat(b, PD_PR_SCALE[scale], sizeof b);
        ui_cat(b, " - ", sizeof b);
        ui_cat(b, PD_PR_KEY[key], sizeof b);
    }
    pa_text(236 - text_w(&FONT_T, b), 0, &FONT_T, b, PD_PR_BLANC, 256);
    pa_rect(0, 18, PA_W, 2, PD_PR_BLANC, 30);   /* the input's level */
    pa_rect(0, 18, level * PA_W >> 14, 2, PD_PR_JAUNE, 220);

    if (heard <= 0) {
        pa_text(4, 19, &FONT_V, "- -", PD_PR_JAUNE, 64);
        pa_text(PD_PR_TX, 30, &FONT_T, "SILENCE", PD_PR_BLANC, 110);
    } else {
        const int32_t off = pd_pr_off(heard, accord ? 0 : scale, key), wrong = pa_abs(off) > 20;
        const int32_t mark = pa_clamp((off + 50) * 20 / 100, 0, 20);
        pd_pr_name(b, (heard + 50) / 100);
        if (text_w(&FONT_V, b) <= PD_PR_TX - 8)
            pa_text(2, 19, &FONT_V, b, PD_PR_JAUNE, 256);
        else
            pa_text(2, 30, &FONT_M, b, PD_PR_JAUNE, 256);
        for (i = 0; i < 21; i++) {          /* a semitone in 21 cells, the middle JUSTE */
            x = PD_PR_TX + i * PD_PR_CELL;
            if (i == mark)
                pa_rect(x, 22, PD_PR_CELL - 1, 10, wrong ? PD_PR_ROUGE : PD_PR_CYAN, 256);
            else if (i == 10)
                pa_rect(x + 1, 24, PD_PR_CELL - 3, 6, PD_PR_CYAN, 256);
            else
                pa_rect(x, 22, PD_PR_CELL - 1, 10, PD_PR_BLANC, 40);
        }
        if (wrong)
            pd_chip(PD_PR_TX, 34, &FONT_T, "FAUX !", PD_PR_ROUGE, PD_PR_BLACK);
        else
            pa_text(PD_PR_TX, 34, &FONT_T, "JUSTE", PD_PR_CYAN, 256);
        b[0] = 0;
        ui_cat(b, off > 0 ? "+" : "", sizeof b);
        ui_cat_int(b, off, sizeof b);
        pa_text(236 - text_w(&FONT_T, b), 34, &FONT_T, b, PD_PR_BLANC, 170);
    }
    if (!accord) {                          /* the note it sends */
        x = pa_text(PD_PR_TX, 49, &FONT_T, "ENVOI", PD_PR_BLANC, 150);
        if (sent > 0)
            pd_pr_name(b, sent - 1);
        else
            str_cpy(b, "-", sizeof b);
        pa_text(x + 8, 49, &FONT_T, b, PD_PR_CYAN, sent > 0 ? 256 : 90);
    }

    for (i = 0; i < 4; i++) {               /* the choir: each voice's level and the note it sings */
        const int32_t pitch = pa_live(p, 3u + (uint32_t)i), lv = pa_max(pa_live(p, 7u + (uint32_t)i), 0);
        const int32_t sings = pitch > 0 && lv > 819;
        x = 2 + i * 59;
        w = lv * 16 >> 14;
        pa_rect(x, 67, 4, 16, PD_PR_VOICE[i], 50);
        pa_rect(x, 83 - w, 4, w, PD_PR_VOICE[i], 256);
        if (sings)
            pd_pr_name(b, (pitch + 50) / 100);
        else
            str_cpy(b, "-", sizeof b);
        pa_text(x + 8, 66, &FONT_T, b, PD_PR_VOICE[i], sings ? 256 : 90);
    }
    cv_blit(FB_PIC_X, FB_PIC_Y);
}

/* ==== KODAMA (KAIRO): v0 head 1 in ms, v1 note length + 1 while synced (0 free), v2 intensity (0..1.5), v3 heads
   (0..6: 1, 2, 3, 1+2, 2+3, 1+3, 1+2+3), v8 spring decay (log 0.5..6 s), the rest for the tiles; live: head 1 in ms,
   speed off true (half way true, +-1 % at the ends), record level, the three heads' levels, the springs' level, the
   tape's travel. KAIRO's gold on the lacquer, labelled in Japanese as Steckfeld does: the name down the left, こだま
   and 木霊; the tape loop running round past the erase head 消, the record head 録 and the playback heads 一 二 三, a
   playing head solid and glowing as loud as it plays, a silent one an outline; the capstan turning at the bottom, the
   speed needle 速度 inside; right the echo time, the heads 頭 and the intensity 強 (自励 in red past 100 %, where the
   tape howls by itself) and the spring tank 発条, its coil shaking as loud as the springs ring */
#define PD_KO_GOLD RGB(240, 176, 56)
#define PD_KO_BRIGHT RGB(255, 228, 140)
#define PD_KO_SHU RGB(240, 56, 30)
#define PD_KO_CX 62
#define PD_KO_CY 43
#define PD_KO_R 25
#define PD_KO_PX 116                        /* the right panel */
static const char* const PD_KO_HEADS[7] = { "1", "2", "3", "1+2", "2+3", "1+3", "1+2+3" };
static const char* const PD_KO_DIV[10] = { "1/2", "1/4", "1/4D", "1/4T", "1/8", "1/8D", "1/8T", "1/16", "1/16T", "1/32" };
static const uint8_t PD_KO_MASK[7] = { 1, 2, 4, 3, 6, 5, 7 };

static uint32_t pd_deg(int32_t d) { return (uint32_t)(d * 65536 / 360); }

/* a point r px from the loop's centre at phase ph, Q4 */
static int32_t pd_ko_x(int32_t r16, uint32_t ph) { return PD_KO_CX * 16 + ((r16 * pc_cos(ph)) >> 15); }
static int32_t pd_ko_y(int32_t r16, uint32_t ph) { return PD_KO_CY * 16 + ((r16 * pc_sin(ph)) >> 15); }

/* a head against the tape at d degrees (clockwise from the right): lit -1 an outline, else solid with a glow of lit /256 */
static void pd_ko_head(int32_t d, const char* label, int32_t lit)
{
    const uint32_t ph = pd_deg(d);
    const int32_t cx = pd_ko_x((PD_KO_R + 7) * 16, ph), cy = pd_ko_y((PD_KO_R + 7) * 16, ph);
    const int32_t tx = -pc_sin(ph), ty = pc_cos(ph), nx = pc_cos(ph), ny = pc_sin(ph);   /* Q15 */
    int32_t px[4], py[4], k, s;
    for (s = lit > 8 ? 0 : 1; s < 2; s++) {   /* the glow, then the block */
        const int32_t a = s ? 5 * 16 : 7 * 16, b = s ? 3 * 16 : 5 * 16;
        px[0] = cx + ((a * tx + b * nx) >> 15);
        py[0] = cy + ((a * ty + b * ny) >> 15);
        px[1] = cx + ((a * tx - b * nx) >> 15);
        py[1] = cy + ((a * ty - b * ny) >> 15);
        px[2] = cx + ((-a * tx - b * nx) >> 15);
        py[2] = cy + ((-a * ty - b * ny) >> 15);
        px[3] = cx + ((-a * tx + b * nx) >> 15);
        py[3] = cy + ((-a * ty + b * ny) >> 15);
        if (!s)
            pc_poly(px, py, 4, PD_KO_BRIGHT, lit * 140 >> 8);
        else if (lit >= 0)
            pc_poly(px, py, 4, PD_KO_GOLD, 256);
        else
            for (k = 0; k < 4; k++)
                pa_line(px[k], py[k], px[(k + 1) & 3], py[(k + 1) & 3], PD_KO_GOLD, 150, 0);
    }
    pc_kanji((pd_ko_x((PD_KO_R + 15) * 16, ph) >> 4) - 6, (pd_ko_y((PD_KO_R + 15) * 16, ph) >> 4) - 6, 12, 1, label,
             lit >= 0 ? PD_KO_BRIGHT : PD_KO_GOLD, lit >= 0 ? 256 : 150);
}

static void pd_kodama(const fb_pic_t* p)
{
    const int32_t heads = pa_min(pa_v(p, 3), 6), synced = pa_v(p, 1), intensity = (pa_v(p, 2) * 150 + 8191) / 16383;
    const int32_t ms = pa_live(p, 0) >= 0 ? pa_live(p, 0) : pa_v(p, 0);
    const int32_t wobble = pa_live(p, 1) >= 0 ? pa_live(p, 1) - 8192 : 0;
    const int32_t travel = pa_max(pa_live(p, 7), 0), spring = pa_max(pa_live(p, 6), 0);
    const uint32_t run = (uint32_t)travel * 2u / 3u;   /* a head-1 span of tape is a sixth of the loop */
    const int32_t gx = PD_KO_CX * 16, gy = (PD_KO_CY + 3) * 16;
    const felucca_font_t* f;
    char b[24];
    int32_t i, x, y, w, amp;
    cv_begin(FB_PIC_W, FB_PIC_H, RGB(5, 4, 16));

    pc_kanji_glow(0, 1, 16, 1, "こ", PD_KO_BRIGHT, PD_KO_GOLD);   /* the name down the left, then 木霊 */
    pc_kanji_glow(0, 18, 16, 1, "だ", PD_KO_BRIGHT, PD_KO_GOLD);
    pc_kanji_glow(0, 35, 16, 1, "ま", PD_KO_BRIGHT, PD_KO_GOLD);
    pc_kanji(2, 57, 12, 1, "木", PD_KO_GOLD, 256);
    pc_kanji(2, 70, 12, 1, "霊", PD_KO_GOLD, 256);

    pc_ring(PD_KO_CX, PD_KO_CY, PD_KO_R + 3, PD_KO_GOLD, 200);   /* the tape: its edges, dashes running round */
    pc_ring(PD_KO_CX, PD_KO_CY, PD_KO_R - 3, PD_KO_GOLD, 200);
    for (i = 0; i < 48; i++) {
        const uint32_t ph = (uint32_t)i * 65536u / 48u + run;
        pa_rect((pd_ko_x(PD_KO_R * 16, ph) >> 4) - 1, (pd_ko_y(PD_KO_R * 16, ph) >> 4) - 1, 2, 2, PD_KO_GOLD, 170);
    }
    pd_ko_head(150, "消", -1);
    pd_ko_head(185, "録", pa_max(pa_live(p, 2), 0) >> 6);
    for (i = 0; i < 3; i++) {
        static const char* const L[3] = { "一", "二", "三" };
        pd_ko_head(245 + 60 * i, L[i], (PD_KO_MASK[heads] >> i) & 1u ? pa_min(pa_max(pa_live(p, 3u + (uint32_t)i), 0) * 3, 16383) >> 6 : -1);
    }
    x = PD_KO_CX;                           /* the capstan, turning with the tape */
    y = PD_KO_CY + PD_KO_R + 9;
    pc_ring(x, y, 5, PD_KO_GOLD, 256);
    for (i = 0; i < 3; i++) {
        const uint32_t ph = (uint32_t)travel * 4u + (uint32_t)i * 21845u;
        pa_line(x * 16 + 8, y * 16 + 8, x * 16 + 8 + ((4 * 16 * pc_cos(ph)) >> 15), y * 16 + 8 + ((4 * 16 * pc_sin(ph)) >> 15),
                PD_KO_GOLD, 256, 0);
    }

    for (i = -2; i <= 2; i++) {             /* the speed: +-1 % across the arc, true at the top */
        const uint32_t ph = pd_deg(270 + 30 * i);
        pa_line(gx + ((13 * 16 * pc_cos(ph)) >> 15), gy + ((13 * 16 * pc_sin(ph)) >> 15), gx + (((i ? 10 : 8) * 16 * pc_cos(ph)) >> 15),
                gy + (((i ? 10 : 8) * 16 * pc_sin(ph)) >> 15), PD_KO_GOLD, 200, 0);
    }
    {
        const uint32_t ph = pd_deg(270) + (uint32_t)(pa_clamp(wobble, -8192, 8192) * 2731 / 8192 * 4);   /* 60 degrees */
        pa_line(gx, gy, gx + ((12 * 16 * pc_cos(ph)) >> 15), gy + ((12 * 16 * pc_sin(ph)) >> 15), 0xFFFFu, 256, 0);
        pa_rect((gx >> 4) - 1, (gy >> 4) - 1, 3, 3, PD_KO_GOLD, 256);
    }
    pc_kanji(PD_KO_CX - 12, PD_KO_CY + 6, 12, 1, "速度", PD_KO_GOLD, 220);

    b[0] = 0;                               /* the echo time: the note length while synced, else ms */
    if (synced)
        ui_cat(b, PD_KO_DIV[pa_min(synced - 1, 9)], sizeof b);
    else
        ui_cat_int(b, ms, sizeof b);
    f = text_w(&FONT_V, b) + (synced ? 0 : 24) <= 236 - PD_KO_PX ? &FONT_V : &FONT_M;
    x = pa_text(PD_KO_PX, f == &FONT_V ? -4 : 6, f, b, 0xFFFFu, 256);
    if (!synced)
        pa_text(x + 3, 18, &FONT_T, "ms", PD_KO_GOLD, 256);

    if (intensity <= 100) {                 /* the heads (the loop shows them too: 自励 takes their room) */
        x = pc_kanji(PD_KO_PX, 44, 12, 1, "頭", PD_KO_GOLD, 256);
        pa_text(x + 3, 41, &FONT_T, PD_KO_HEADS[heads], PD_KO_BRIGHT, 256);
    }
    b[0] = 0;                               /* the intensity; past 100 % 自励, the tape howls by itself */
    ui_cat_int(b, intensity, sizeof b);
    ui_cat(b, "%", sizeof b);
    w = 236 - text_w(&FONT_T, b);
    pa_text(w, 41, &FONT_T, b, intensity > 100 ? PD_KO_SHU : PD_KO_BRIGHT, 256);
    if (intensity > 100)
        pc_kanji(w - 27, 44, 12, 1, "自励", PD_KO_SHU, 256);
    else
        pc_kanji(w - 15, 44, 12, 1, "強", PD_KO_GOLD, 256);

    pc_frame(PD_KO_PX, 62, 236 - PD_KO_PX, 21, PD_KO_GOLD, 150);   /* the spring tank */
    pc_kanji(PD_KO_PX + 3, 66, 12, 1, "発条", PD_KO_GOLD, 256);
    amp = pa_min(spring * 3, 16383) * 3 >> 14;
    for (x = PD_KO_PX + 30; x < 194; x++) {
        const int32_t s = x - PD_KO_PX - 30;
        const int32_t jit = amp * pc_sin((uint32_t)s * 3100u + fm1_ms * 2600u) >> 15;
        y = 72 * 16 + ((6 * 16 * pc_sin((uint32_t)s * 9000u)) >> 15) + jit * 16 * pc_sin((uint32_t)s * 65536u / 48u / 2u) / 32768;
        pa_rect(x, y >> 4, 1, 1, PD_KO_BRIGHT, 220);
        pa_px(x, (y >> 4) + 1, PD_KO_BRIGHT, (y & 15) * 220 >> 4);
    }
    pc_ring(196, 72, 2, PD_KO_GOLD, 256);
    w = (pa_exp2(pa_v(p, 8) * 14684 / 16383) * 5u + 32768u) >> 16;   /* 0.5 * 12^v, in tenths */
    b[0] = 0;
    ui_cat_int(b, w / 10, sizeof b);
    ui_cat(b, ".", sizeof b);
    ui_cat_int(b, w % 10, sizeof b);
    ui_cat(b, "s", sizeof b);
    pa_text(234 - text_w(&FONT_S, b), 64, &FONT_S, b, PD_KO_GOLD, 256);
    cv_blit(FB_PIC_X, FB_PIC_Y);
}

/* ---- RASTER's sheet: SCHLIERE (part C) is view A with its scale field; MITSCHNITT gets a title block with its
   indicator lamps, EMPFANG its lamp and two bargraph instruments, all as a circuit diagram draws them */
#define PD_RA_RED RGB(255, 46, 31)
#define PD_RA_LAMP 5                        /* a lamp's radius */

/* a lamp: a circle with a cross, filled in c at lit /256 behind the cross (0: dark) */
static void pd_ra_lamp(int32_t cx, int32_t cy, uint16_t c, int32_t lit)
{
    const int32_t r = PD_RA_LAMP, d = r * 181 >> 8;   /* r / sqrt 2 */
    int32_t y;
    if (lit > 0)
        for (y = -r; y <= r; y++) {
            const int32_t w = pa_isqrt((uint32_t)(r * r - y * y) * 256u);
            pa_hspan(cy + y, cx * 16 + 8 - w, cx * 16 + 8 + w, c, lit);
        }
    pc_ring(cx, cy, r, lit > 0 ? c : 0xFFFFu, lit > 0 ? 256 : PC_RA_LINE);
    pa_line(cx * 16 + 8 - d * 16, cy * 16 + 8 - d * 16, cx * 16 + 8 + d * 16, cy * 16 + 8 + d * 16,
            lit > 0 ? RGB(14, 16, 20) : 0xFFFFu, lit > 0 ? 200 : PC_RA_LINE, 0);
    pa_line(cx * 16 + 8 - d * 16, cy * 16 + 8 + d * 16, cx * 16 + 8 + d * 16, cy * 16 + 8 - d * 16,
            lit > 0 ? RGB(14, 16, 20) : 0xFFFFu, lit > 0 ? 200 : PC_RA_LINE, 0);
}

/* ==== MITSCHNITT (RASTER): v0 loop (0 off, 1 forward, 2 ping-pong), v1 warp, v2 root note, v4 start, v5 end, v6 loop
   start, v7 loop end (both of start..end), v11 the take's length in ms, v12 threshold (-70 off .. -6 dB), v15 record
   mode (0 FWD, 1 FWD KEEP, 2 BACK, 3 BACK KEEP), the rest for the tiles; arr the take's outline, 128 peaks; live:
   state (0 idle, 1 armed, 2 recording, 4 listening: BACK's buffer running), ms recorded (listening: buffered), input
   peak, 4 voices' place, 4 voices' gain. RASTER's sheet, in German: the state and the record mode on the top line;
   the take mirrored about its axis, gold between Start (A) and End (E), each voice playing a line, the loop
   dimensioned under it (one arrowhead forward, two ping-pong); along the foot a title block with the root, the
   take's length and the lamps H1 (red: recording, blinking armed) and H2..H5 (a voice each, gold as loud as it
   plays). Armed or recording: the input's level against the threshold and the take filling up towards 12 s */
#define PD_RA_X0 4
#define PD_RA_W 232
#define PD_MI_TOP 29                        /* the take */
#define PD_MI_MID 43
#define PD_MI_HALF 13
#define PD_MI_BLOCK 66                      /* the title block */
#define PD_MI_MAX_MS 12000                  /* a take holds 12 s at most (SYNTH_SAMPLER_SECONDS) */
static const char* const PD_MI_MODE[4] = { "FWD", "FWD KEEP", "BACK", "BACK KEEP" };

static int32_t pd_mi_x(int32_t q14) { return PD_RA_X0 + (pa_clamp(q14, 0, 16384) * PD_RA_W >> 14); }

static void pd_mi_take(const fb_pic_t* p)
{
    const int32_t start = pa_v(p, 4), end = pa_max(pa_v(p, 5), start), span = end - start, loop = pa_v(p, 0);
    int32_t x, i, xa;
    for (x = PD_RA_X0; x < PD_RA_X0 + PD_RA_W; x++) {   /* the outline, bright inside Start and End */
        const int32_t s = (x - PD_RA_X0) * 128 / PD_RA_W, f = (2 * s + 1) * 64;
        const int32_t h = pa_max((s < (int32_t)p->arr_n ? p->arr[s] : 0) * PD_MI_HALF * 16 / 127, 8);
        const int in = f >= start && f <= end;
        pa_vspan(x, PD_MI_MID * 16 - h, PD_MI_MID * 16 + h, in ? PC_RA_GOLD : 0xFFFFu, in ? 256 : 60);
    }
    pa_hdash(PD_MI_MID, PD_RA_X0, PD_RA_X0 + PD_RA_W - 1, 0xFFFFu, PC_RA_FAINT);   /* the axis, a centre line */
    for (i = 0; i < 2; i++) {               /* Start and End, flagged */
        const char* l = i ? "E" : "A";
        const int32_t w = text_w(&FONT_S, l) + 4;
        x = pd_mi_x(i ? end : start);
        xa = i ? x - w + 1 : x;
        pa_rect(x, PD_MI_TOP - 11, 1, PD_MI_HALF * 2 + 16, 0xFFFFu, 230);
        pa_rect(xa, PD_MI_TOP - 12, w, 12, 0xFFFFu, 230);
        pa_text(xa + 2, PD_MI_TOP - 13, &FONT_S, l, RGB(14, 16, 20), 256);
    }
    if (loop) {                             /* the loop, dimensioned under the take */
        const int32_t a = pd_mi_x(start + (pa_v(p, 6) * span >> 14)), b = pd_mi_x(start + (pa_v(p, 7) * span >> 14));
        const int32_t y = (PD_MI_MID + PD_MI_HALF + 5) * 16 + 8;
        pa_rect(a, PD_MI_MID + PD_MI_HALF + 1, 1, 7, 0xFFFFu, PC_RA_LINE);
        pa_rect(b, PD_MI_MID + PD_MI_HALF + 1, 1, 7, 0xFFFFu, PC_RA_LINE);
        if (loop >= 2 && b - a >= 12) {     /* as SCHLIERE dimensions its glide */
            pc_ra_dim(a * 16 + 8, y, b * 16 + 8, y);
        } else {
            pa_line(a * 16 + 8, y, b * 16 + 8, y, 0xFFFFu, PC_RA_LINE, 0);
            if (b - a >= 6)
                pc_ra_arrow(b * 16 + 8, y, 1, 0, PC_RA_LINE);
        }
    }
    for (i = 0; i < 4; i++) {               /* each voice playing, as bright as loud */
        const int32_t gain = pa_live(p, 7u + (uint32_t)i);
        if (gain > 0) {
            x = pd_mi_x(pa_max(pa_live(p, 3u + (uint32_t)i), 0));
            pa_rect(x, PD_MI_TOP - 2, 1, PD_MI_HALF * 2 + 4, 0xFFFFu, 90 + (gain * 166 >> 14));
            pa_rect(x - 1, PD_MI_TOP - 2, 3, 2, 0xFFFFu, 256);
        }
    }
}

/* armed or recording: the input's peak in dB from -60 with the threshold, and the take filling up to 12 s */
static void pd_mi_rec(const fb_pic_t* p)
{
    const int32_t db = pd_db10(pa_max(pa_live(p, 2), 0)), thr = pa_v(p, 12);
    const int32_t thr_db = -700 + thr * 640 / 16383, rec = pa_max(pa_live(p, 1), 0);
    char b[8];
    int32_t i, x, w;
    w = pa_clamp((db + 600) * PD_RA_W / 600, 0, PD_RA_W);
    pc_frame(PD_RA_X0, 20, PD_RA_W, 12, 0xFFFFu, PC_RA_LINE);
    pa_rect(PD_RA_X0 + 1, 21, pa_max(w - 2, 0), 10, db > -60 ? PD_RA_RED : PC_RA_GOLD, 256);
    for (i = -60; i <= 0; i += 12) {
        x = PD_RA_X0 + (i + 60) * PD_RA_W / 60;
        b[0] = 0;
        ui_cat_int(b, i, sizeof b);
        pa_rect(pa_min(x, PD_RA_X0 + PD_RA_W - 1), 32, 1, 3, 0xFFFFu, PC_RA_FAINT);
        pa_text(pa_clamp(x - text_w(&FONT_S, b) / 2, 0, 238 - text_w(&FONT_S, b)), 33, &FONT_S, b, 0xFFFFu, PC_RA_FAINT);
    }
    if (thr) {                              /* the threshold: a take starts there */
        x = PD_RA_X0 + pa_clamp((thr_db + 600) * PD_RA_W / 600, 0, PD_RA_W - 2);
        pa_rect(x, 17, 2, 18, 0xFFFFu, 256);
    }
    pc_frame(PD_RA_X0, 51, PD_RA_W, 11, PC_RA_GOLD, 256);   /* the take so far against the 12 s it holds */
    pa_rect(PD_RA_X0 + 2, 53, pa_clamp(rec * (PD_RA_W - 4) / PD_MI_MAX_MS, 0, PD_RA_W - 4), 7, PD_RA_RED, 256);
}

/* the title block along the foot: left (gold) and mid, each closed by a rule, then the lamps */
static void pd_mi_block(const fb_pic_t* p, const char* left, const char* mid, int32_t state)
{
    int32_t x, i;
    pc_frame(0, PD_MI_BLOCK, PA_W, PA_H - PD_MI_BLOCK, 0xFFFFu, PC_RA_LINE);
    x = pa_text(5, PD_MI_BLOCK + 1, &FONT_S, left, PC_RA_GOLD, 256) + 5;
    pa_rect(x, PD_MI_BLOCK, 1, PA_H - PD_MI_BLOCK, 0xFFFFu, PC_RA_FAINT);
    if (mid[0]) {
        x = pa_text(x + 6, PD_MI_BLOCK + 1, &FONT_S, mid, 0xFFFFu, 230) + 5;
        pa_rect(x, PD_MI_BLOCK, 1, PA_H - PD_MI_BLOCK, 0xFFFFu, PC_RA_FAINT);
    }
    x = PA_W - 5 * 14 - 1;
    pa_rect(x - 4, PD_MI_BLOCK, 1, PA_H - PD_MI_BLOCK, 0xFFFFu, PC_RA_FAINT);
    pd_ra_lamp(x + 6, PD_MI_BLOCK + 9, PD_RA_RED, state == 2 || (state == 1 && (fm1_ms / 400u) & 1u) ? 256 : 0);
    for (i = 0; i < 4; i++) {
        const int32_t gain = pa_live(p, 7u + (uint32_t)i);
        pd_ra_lamp(x + 6 + 14 * (i + 1), PD_MI_BLOCK + 9, PC_RA_GOLD, gain > 0 ? 90 + (gain * 166 >> 14) : 0);
    }
}

static void pd_mitschnitt(const fb_pic_t* p)
{
    const int32_t live = pa_max(pa_live(p, 0), 0), length = pa_v(p, 11), thr = pa_v(p, 12);
    const int32_t state = live == 1 || live == 2 ? live : 0, mode = pa_min(pa_v(p, 15), 3);   /* listening plays as idle */
    char b[32], c[16];
    cv_begin(FB_PIC_W, FB_PIC_H, RGB(14, 16, 20));
    if (state == 2) {
        pd_chip(0, 0, &FONT_S, "AUFNAHME", PD_RA_RED, 0);
    } else if (state == 1) {
        if ((fm1_ms / 400u) & 1u)
            pd_chip(0, 0, &FONT_S, "SCHARF", PD_RA_RED, 0);
    } else if (live == 4) {                 /* BACK: the buffer runs, GRAB takes it */
        str_cpy(b, "PUFFER ", sizeof b);
        ui_cat_int(b, (pa_max(pa_live(p, 1), 0) + 500) / 1000, sizeof b);
        ui_cat(b, " s", sizeof b);
        pd_chip(0, 0, &FONT_S, b, PC_RA_GOLD, 0);
    } else {
        pd_chip(0, 0, &FONT_S, length ? "BEREIT" : "LEER", length ? PC_RA_GOLD : RGB(110, 112, 118), 0);
    }
    str_cpy(b, PD_MI_MODE[mode], sizeof b);
    if (pa_v(p, 1))
        ui_cat(b, " \xB7 WARP", sizeof b);
    pa_text(238 - text_w(&FONT_S, b), 0, &FONT_S, b, 0xFFFFu, 220);

    if (state) {
        pd_mi_rec(p);
    } else if (!length) {
        pa_text(120 - text_w(&FONT_M, "KEIN MITSCHNITT") / 2, 16, &FONT_M, "KEIN MITSCHNITT", 0xFFFFu, PC_RA_FAINT);
        pa_text(120 - text_w(&FONT_S, "Eingang verkabeln,") / 2, 37, &FONT_S, "Eingang verkabeln,", 0xFFFFu, PC_RA_FAINT);
        str_cpy(b, mode >= 2 ? "GRAB dr\xFC" "cken" : "REC dr\xFC" "cken", sizeof b);
        pa_text(120 - text_w(&FONT_S, b) / 2, 50, &FONT_S, b, 0xFFFFu, PC_RA_FAINT);
    } else {
        pd_mi_take(p);
    }
    if (state == 1 && thr) {                /* the foot: what it waits for, or the root and the take's length */
        str_cpy(b, "WARTET AUF ", sizeof b);
        ui_cat_int(b, (-700 + thr * 640 / 16383 - 5) / 10, sizeof b);
        ui_cat(b, " dB", sizeof b);
        pd_mi_block(p, b, "", state);
        cv_blit(FB_PIC_X, FB_PIC_Y);
        return;
    }
    str_cpy(b, "GRUNDTON ", sizeof b);
    pb_note_name(c, pa_min(pa_v(p, 2), 127));
    ui_cat(b, c, sizeof b);
    if (state == 2)
        pd_secs(c, pa_max(pa_live(p, 1), 0), 1);
    else if (state == 1)
        str_cpy(c, "STARTET", sizeof c);
    else if (length)
        pd_secs(c, length, 2);
    else
        str_cpy(c, "-", sizeof c);
    pd_mi_block(p, b, c, state);
    cv_blit(FB_PIC_X, FB_PIC_Y);
}

/* ==== EMPFANG (RASTER): v0 station (0-based), v1 stations, v2 volume, v3 S1 (1 EIN, 0 AUS; missing: EIN); arr the
   station's name str and its genre str; live: the line (0 off, 1 dialling, 2 live, 3 dialling again, 4 no network),
   1 while audio plays, level L, level R (before the volume), ms buffered. RASTER's sheet as a circuit diagram: the
   line's lamp H1 (steady gold while the station plays, blinking while it dials, red with no network) and its state,
   the station's place on the top line, the station's name large and its genre, then the bargraph instruments P1 L
   and P2 R, twelve cells of 4 dB from -48 dB, the last two red. S1 at AUS: the line hung up, the name faint and S1
   AUS where the instruments were */
#define PD_EM_CELLS 12
#define PD_EM_X0 40                         /* the instruments' boxes */
#define PD_EM_CELL 16

static void pd_em_meter(int32_t y, const char* name, int32_t level)
{
    const int32_t db = pd_db10(level), reached = level > 0 ? (db + 480) * PD_EM_CELLS / 480 : 0;
    int32_t k;
    pa_text(3, y - 4, &FONT_S, name, 0xFFFFu, 220);
    for (k = 0; k < PD_EM_CELLS; k++) {
        if (k < reached)
            pa_rect(PD_EM_X0 + k * PD_EM_CELL + 2, y + 2, PD_EM_CELL - 3, 5, k >= PD_EM_CELLS - 2 ? PD_RA_RED : PC_RA_GOLD, 256);
        if (k)
            pa_rect(PD_EM_X0 + k * PD_EM_CELL, y, 1, 9, 0xFFFFu, PC_RA_FAINT);
    }
    pc_frame(PD_EM_X0, y, PD_EM_CELLS * PD_EM_CELL + 1, 9, 0xFFFFu, PC_RA_LINE);
}

static int pd_em_on(const fb_pic_t* p) { return p->n < 4u || pa_v(p, 3); }

static int pd_em_dialling(const fb_pic_t* p)
{
    const int32_t link = pa_live(p, 0);
    return pd_em_on(p) && (link == 1 || link == 3 || (link == 2 && pa_live(p, 1) <= 0));
}

static void pd_empfang(const fb_pic_t* p)
{
    const int32_t on = pd_em_on(p), link = on ? pa_clamp(pa_live(p, 0), 0, 4) : 0, blink = (fm1_ms / 400u) & 1u;
    const char *name = (const char*)p->arr, *genre = "";
    char b[32], c[16];
    uint32_t i;
    cv_begin(FB_PIC_W, FB_PIC_H, RGB(14, 16, 20));
    pd_ra_lamp(6, 7, link == 4 ? PD_RA_RED : PC_RA_GOLD, link == 4 || (link && (!pd_em_dialling(p) || blink)) ? 256 : 0);
    if (link == 4)
        pd_chip(15, 0, &FONT_S, "KEIN NETZ", PD_RA_RED, 0);
    else if (link == 0)
        pd_chip(15, 0, &FONT_S, "AUS", RGB(110, 112, 118), 0);
    else if (!pd_em_dialling(p))
        pd_chip(15, 0, &FONT_S, "LIVE", PC_RA_GOLD, 0);
    else if (blink)
        pd_chip(15, 0, &FONT_S, link == 3 ? "W\xC4HLT NEU" : "W\xC4HLT", PC_RA_GOLD, 0);
    str_cpy(b, "SENDER ", sizeof b);
    ui_cat_int(b, pa_v(p, 0) + 1, sizeof b);
    ui_cat(b, " VON ", sizeof b);
    ui_cat_int(b, pa_v(p, 1), sizeof b);
    pa_text(238 - text_w(&FONT_S, b), 0, &FONT_S, b, 0xFFFFu, 220);

    for (i = 0; i < p->arr_n && p->arr[i]; i++)   /* the two strs; an unended one shows nothing */
        ;
    if (i < p->arr_n) {
        genre = (const char*)p->arr + i + 1;
        for (i++; i < p->arr_n && p->arr[i]; i++)
            ;
    }
    if (i >= p->arr_n)
        name = genre = "";
    if (text_w(&FONT_L, name) <= 236)
        pa_text(2, 16, &FONT_L, name, PC_RA_GOLD, on ? 256 : 90);
    else
        pa_text(2, 22, &FONT_M, name, PC_RA_GOLD, on ? 256 : 90);
    pa_text(2, 47, &FONT_S, genre, 0xFFFFu, on ? 220 : PC_RA_FAINT);
    if (!on) {
        pa_text(120 - text_w(&FONT_M, "S1 AUS") / 2, 61, &FONT_M, "S1 AUS", 0xFFFFu, 240);
        cv_blit(FB_PIC_X, FB_PIC_Y);
        return;
    }
    if (link) {
        str_cpy(b, "PUFFER ", sizeof b);
        pd_secs(c, pa_max(pa_live(p, 4), 0), 1);
        ui_cat(b, c, sizeof b);
        pa_text(238 - text_w(&FONT_S, b), 47, &FONT_S, b, 0xFFFFu, PC_RA_FAINT);
    }
    pd_em_meter(64, "P1 L", pa_max(pa_live(p, 2), 0));
    pd_em_meter(75, "P2 R", pa_max(pa_live(p, 3), 0));
    cv_blit(FB_PIC_X, FB_PIC_Y);
}

/* ---- the drawer: 1 if pic is one of part D's kinds (drawn and blitted), else 0 */
static int fb_pics_d_draw(const fb_pic_t* pic)
{
    switch (pic->kind) {
    case FB_K_DRUM: pd_buben(pic); return 1;
    case FB_K_PRISE: pd_prise(pic); return 1;
    case FB_K_KODAMA: pd_kodama(pic); return 1;
    case FB_K_SAMPLER: pd_mitschnitt(pic); return 1;
    case FB_K_RADIO: pd_empfang(pic); return 1;
    default: return 0;
    }
}

/* 1 while pic moves by itself: MITSCHNITT armed and EMPFANG dialling blink */
static int fb_pics_d_moves(const fb_pic_t* pic)
{
    return (pic->kind == FB_K_SAMPLER && pa_live(pic, 0) == 1) || (pic->kind == FB_K_RADIO && pd_em_dialling(pic));
}

#endif
