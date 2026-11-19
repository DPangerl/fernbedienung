/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Daniel Pangerl */
/* Fernbedienung's side of the protocol (docs/fernbedienung/protocol.md; unity build, included once by app.c): what
 * Steckfeld sent, parsed from app_sysex (main loop, between frames) into the state the screen is drawn from, and
 * HELLO back. A message is taken whole or not at all: a short or malformed one changes nothing. */
#ifndef FB_LINK_H
#define FB_LINK_H

#define FB_PAGES_MAX 8u
#define FB_LIST_ROWS 8u                     /* rows kept of a LIST (the window around the selection) */
#define FB_TITLE_N 24u
#define FB_LABEL_N 14u
#define FB_TEXT_N 24u

/* dirty regions: what the next frame redraws */
#define FB_D_HEAD 1u
#define FB_D_PIC 2u
#define FB_D_TILES 0x3Cu                    /* tile i: 4 << i */
#define FB_D_FOOT 0x40u
#define FB_D_ALL 0x7Fu

enum { FB_LINK_WAIT, FB_LINK_ON, FB_LINK_OLD_FM1, FB_LINK_OLD_APP };

typedef struct {
    uint8_t cat, mode, kind, page, pages, flags;
    char title[FB_TITLE_N];
    char page_title[FB_PAGES_MAX][FB_LABEL_N];
    char rec[FB_LABEL_N], save[FB_LABEL_N];
} fb_screen_t;

typedef struct {
    char text[FB_TITLE_N + 8u];             /* empty: none */
    char act[FB_LABEL_N];                   /* what REC does about it, empty: nothing */
} fb_notice_t;

#define FB_SCR_UNSAVED 1u                   /* the macro: the rack has changes not yet snapshotted */
#define FB_SCR_RUNNING 2u

#define FB_CTL_PRESENT 1u
#define FB_CTL_LIVE 2u
#define FB_CTL_NOBAR 4u                     /* local tiles: no position */

typedef struct {
    uint8_t flags;                          /* FB_CTL_*: bit 0 present, bit 1 live follows */
    uint16_t pos, live;
    char title[FB_LABEL_N + 4u];
    char readout[FB_LABEL_N + 4u];
} fb_ctl_t;

typedef struct {
    uint8_t flags;                          /* bit 0 on, bit 1 outgoing, bit 2 plays a sequence (FB_ROW_SEQ) */
    char text[FB_TEXT_N], detail[FB_TEXT_N];
} fb_row_t;

typedef struct {
    uint16_t sel, first;
    uint8_t open, count;                    /* count: rows kept */
    char title[FB_LABEL_N];                 /* the list's filter (patches) */
    fb_row_t row[FB_LIST_ROWS];
} fb_list_t;

static struct {
    uint32_t link;                          /* FB_LINK_* */
    uint32_t app_version, app_theme;        /* from ACK */
    uint32_t dirty;
    fb_screen_t scr;
    fb_ctl_t ctl[4];
    fb_pic_t pic;
    fb_list_t list[2];                      /* 0 cables (Route), 1 patches (the overlay) */
    fb_notice_t notice;
    uint32_t bay;                           /* BAY: bit n, category n has a module (all until the app says) */
    uint32_t rx, bad;                       /* messages taken, refused */
} fb;

/* ---- reading a message body */
typedef struct {
    const uint8_t *p, *end;
    int bad;
} fb_rd_t;

static uint32_t rd_byte(fb_rd_t* r)
{
    if (r->p >= r->end) {
        r->bad = 1;
        return 0;
    }
    return *r->p++;
}

static uint32_t rd_v14(fb_rd_t* r)
{
    const uint32_t hi = rd_byte(r);
    return hi << 7 | rd_byte(r);
}

/* a str into d (max bytes with its end), cut where it does not fit; anything outside 0x20..0x7E reads as '?' */
static void rd_str(fb_rd_t* r, char* d, uint32_t max)
{
    uint32_t n = 0, c;
    while ((c = rd_byte(r)) != 0 && !r->bad) {
        if (n + 1u < max)
            d[n++] = c >= 0x20u && c <= 0x7Eu ? (char)c : '?';
    }
    if (max)
        d[n] = 0;
}

static void rd_skip_str(fb_rd_t* r)
{
    while (rd_byte(r) != 0 && !r->bad)
        ;
}

/* ---- the messages */
static void fb_on_ack(fb_rd_t* r)
{
    const uint32_t v = rd_byte(r), th = rd_byte(r);
    if (r->bad)
        return;
    fb.app_version = v;
    fb.app_theme = th == 2u ? 2u : 1u;
    fb.bay = (1u << FB_CAT_COUNT) - 1u;
    fb.link = v == FB_PROTO_VERSION ? FB_LINK_ON : v > FB_PROTO_VERSION ? FB_LINK_OLD_FM1 : FB_LINK_OLD_APP;
    fb.dirty = FB_D_ALL;
}

static void fb_on_screen(fb_rd_t* r)
{
    static fb_screen_t s;
    uint32_t i;
    memset(&s, 0, sizeof s);
    s.cat = (uint8_t)rd_byte(r);
    s.mode = (uint8_t)rd_byte(r);
    s.kind = (uint8_t)rd_byte(r);
    s.page = (uint8_t)rd_byte(r);
    s.pages = (uint8_t)rd_byte(r);
    s.flags = (uint8_t)rd_byte(r);
    rd_str(r, s.title, sizeof s.title);
    for (i = 0; i < s.pages && !r->bad; i++) {
        if (i < FB_PAGES_MAX)
            rd_str(r, s.page_title[i], sizeof s.page_title[i]);
        else
            rd_skip_str(r);
    }
    rd_str(r, s.rec, sizeof s.rec);
    rd_str(r, s.save, sizeof s.save);
    if (r->bad)
        return;
    if (s.pages > FB_PAGES_MAX)
        s.pages = FB_PAGES_MAX;
    if (s.mode != fb.scr.mode || s.kind != fb.scr.kind)
        fb.dirty = FB_D_ALL;
    if (memcmp(s.title, fb.scr.title, sizeof s.title) || memcmp(s.page_title, fb.scr.page_title, sizeof s.page_title) ||
        s.page != fb.scr.page || s.pages != fb.scr.pages || s.flags != fb.scr.flags || s.cat != fb.scr.cat)
        fb.dirty |= FB_D_HEAD;
    if (memcmp(s.rec, fb.scr.rec, sizeof s.rec) || memcmp(s.save, fb.scr.save, sizeof s.save))
        fb.dirty |= FB_D_FOOT;
    fb.scr = s;
}

static void fb_on_controls(fb_rd_t* r)
{
    static fb_ctl_t c[4];
    uint32_t i;
    memset(c, 0, sizeof c);
    for (i = 0; i < 4u; i++) {
        c[i].flags = (uint8_t)(rd_byte(r) & 3u);
        c[i].pos = (uint16_t)rd_v14(r);
        if (c[i].flags & 2u)
            c[i].live = (uint16_t)rd_v14(r);
        rd_str(r, c[i].title, sizeof c[i].title);
        rd_str(r, c[i].readout, sizeof c[i].readout);
    }
    if (r->bad)
        return;
    for (i = 0; i < 4u; i++) {
        if (memcmp(&c[i], &fb.ctl[i], sizeof c[i]))
            fb.dirty |= 4u << i;
        fb.ctl[i] = c[i];
    }
}

static void fb_on_picture(fb_rd_t* r)
{
    static fb_pic_t p;
    uint32_t i, n, a;
    p.kind = (uint8_t)rd_byte(r);
    n = rd_byte(r);
    for (i = 0; i < n && !r->bad; i++) {
        const uint32_t v = rd_v14(r);
        if (i < FB_PIC_VALUES)
            p.v[i] = (uint16_t)v;
    }
    p.n = (uint8_t)(n < FB_PIC_VALUES ? n : FB_PIC_VALUES);
    a = rd_v14(r);
    for (i = 0; i < a && !r->bad; i++) {
        const uint32_t b = rd_byte(r);
        if (i < FB_PIC_ARRAY)
            p.arr[i] = (uint8_t)b;
    }
    p.arr_n = (uint16_t)(a < FB_PIC_ARRAY ? a : FB_PIC_ARRAY);
    if (r->bad)
        return;
    p.live_n = p.kind == fb.pic.kind ? fb.pic.live_n : 0;   /* a new kind: the old one's live values mean nothing */
    memcpy(p.live, fb.pic.live, sizeof p.live);
    p.serial = fb.pic.serial + 1u;
    fb.pic = p;
}

static void fb_on_live(fb_rd_t* r)
{
    uint16_t v[FB_LIVE_VALUES];
    const uint32_t kind = rd_byte(r), n = rd_byte(r);
    uint32_t i;
    for (i = 0; i < n && !r->bad; i++) {
        const uint32_t x = rd_v14(r);
        if (i < FB_LIVE_VALUES)
            v[i] = (uint16_t)x;
    }
    if (r->bad || kind != fb.pic.kind)
        return;
    fb.pic.live_n = (uint8_t)(n < FB_LIVE_VALUES ? n : FB_LIVE_VALUES);
    memcpy(fb.pic.live, v, fb.pic.live_n * sizeof v[0]);
    fb.pic.serial++;
}

static void fb_on_list(fb_rd_t* r)
{
    static fb_list_t l;
    const uint32_t type = rd_byte(r);
    uint32_t i, count;
    memset(&l, 0, sizeof l);
    l.sel = (uint16_t)rd_v14(r);
    l.first = (uint16_t)rd_v14(r);
    count = rd_byte(r);
    rd_str(r, l.title, sizeof l.title);
    for (i = 0; i < count && !r->bad; i++) {
        if (i < FB_LIST_ROWS) {
            l.row[i].flags = (uint8_t)(rd_byte(r) & 7u);
            rd_str(r, l.row[i].text, sizeof l.row[i].text);
            rd_str(r, l.row[i].detail, sizeof l.row[i].detail);
        } else {
            (void)rd_byte(r);
            rd_skip_str(r);
            rd_skip_str(r);
        }
    }
    if (r->bad || type > 1u)
        return;
    l.count = (uint8_t)(count < FB_LIST_ROWS ? count : FB_LIST_ROWS);
    l.open = 1;
    if (type == 1u && !fb.list[1].open)
        fb.dirty = FB_D_ALL;                /* the overlay comes up */
    fb.dirty |= FB_D_PIC;
    fb.list[type] = l;
}

static void fb_on_notice(fb_rd_t* r)
{
    fb_notice_t n;
    memset(&n, 0, sizeof n);
    rd_str(r, n.text, sizeof n.text);
    rd_str(r, n.act, sizeof n.act);
    if (r->bad)
        return;
    if (memcmp(&n, &fb.notice, sizeof n))
        fb.dirty |= FB_D_FOOT;
    fb.notice = n;
}

static void fb_on_bay(fb_rd_t* r)
{
    const uint32_t m = rd_v14(r);
    if (!r->bad)
        fb.bay = m & ((1u << FB_CAT_COUNT) - 1u);
}

static void fb_list_close(void)
{
    if (!fb.list[1].open)
        return;
    fb.list[1].open = 0;
    fb.dirty = FB_D_ALL;
}

/* ---- HELLO: protocol version, firmware major, minor, flags, theme (the main loop only: ota_wire_send waits for
   room in the SysEx ring, at most 200 ms) */
static uint32_t fb_ver_part(uint32_t which)
{
    const char* s = FW_VERSION;
    uint32_t v = 0;
    while (which && *s)
        which -= *s++ == '.';
    while (*s >= '0' && *s <= '9')
        v = v * 10u + (uint32_t)(*s++ - '0');
    return v > 127u ? 127u : v;
}

static void fb_send_hello(uint32_t flags, uint32_t theme)
{
    uint8_t m[11];
    m[0] = 0xF0;
    m[1] = FB_SX_ID0;
    m[2] = FB_SX_ID1;
    m[3] = FB_SX_ID2;
    m[4] = FB_CMD_HELLO;
    m[5] = FB_PROTO_VERSION;
    m[6] = (uint8_t)fb_ver_part(0);
    m[7] = (uint8_t)fb_ver_part(1);
    m[8] = (uint8_t)(flags & 127u);
    m[9] = (uint8_t)(theme & 127u);
    m[10] = 0xF7;
    (void)ota_wire_send(m, sizeof m);
}

/* one frame from the host: the 7-bit bytes between F0 and F7. 1: ASK_HELLO (the caller answers) */
static int fb_take(const uint8_t* p, uint32_t n)
{
    fb_rd_t r;
    if (n < 4u || p[0] != FB_SX_ID0 || p[1] != FB_SX_ID1 || p[2] != FB_SX_ID2)
        return 0;
    r.p = p + 4;
    r.end = p + n;
    r.bad = 0;
    switch (p[3]) {
    case FB_CMD_ACK: fb_on_ack(&r); break;
    case FB_CMD_ASK_HELLO:
        fb.link = FB_LINK_WAIT;
        memset(&fb.notice, 0, sizeof fb.notice);
        fb.dirty = FB_D_ALL;
        fb.rx++;
        return 1;
    default:
        if (fb.link != FB_LINK_ON)          /* the state only from an app that speaks our version */
            return 0;
        switch (p[3]) {
        case FB_CMD_SCREEN: fb_on_screen(&r); break;
        case FB_CMD_CONTROLS: fb_on_controls(&r); break;
        case FB_CMD_PICTURE: fb_on_picture(&r); break;
        case FB_CMD_LIVE: fb_on_live(&r); break;
        case FB_CMD_LIST: fb_on_list(&r); break;
        case FB_CMD_LIST_CLOSE: fb_list_close(); break;
        case FB_CMD_NOTICE: fb_on_notice(&r); break;
        case FB_CMD_BAY: fb_on_bay(&r); break;
        default: r.bad = 1; break;
        }
    }
    if (r.bad)
        fb.bad++;
    else
        fb.rx++;
    return 0;
}

#endif
