/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Daniel Pangerl */
/* Fernbedienung: the M-VAVE FM-1 as the controller and USB audio interface of Steckfeld SF1 on an iPad or iPhone.
 * The iOS device plays into the FM-1 over USB (sys/usb.c FW_USB_PLAY, pair 1), MASTER sets the volume. The hands go
 * out over USB-MIDI on the FM-1's channel, the screen shows what Steckfeld sends back as SysEx
 * (docs/fernbedienung/protocol.md):
 *   KNOB1..4            CC 20..23, relative (1 one detent up, 127 one down)
 *   SELECT              CC 24, relative; with a category button held CC 50 + category
 *   ALGORITHM, PRESETS  CC 25, 26, relative
 *   REC, SAVE           CC 27, 28: 127 pressed, 0 released
 *   HOME SCL FX LFO ENV SEQ ARP GLO   tapped: CC 40 + category, 127
 *   PLAY                CC 29: the app runs or stops its transport (the LED follows the app)
 *   keys + OCT          notes (Beiwagen's bw_keys.h)
 *   EDIT                the FM-1's own settings (channel, velocity, when the programmer opens), sent as HELLO with the
 *                       theme (the system settings, HOME + EDIT: sys/settings.c)
 * Built on Beiwagen's screen kit (bw_ui.h: its text and colour helpers). */

#if !FW_USB_PLAY
#error "fernbedienung: build.txt defines FW_USB_PLAY=1"
#endif
#ifndef APP_SYSEX
#error "fernbedienung: build.txt defines APP_SYSEX"
#endif

#include "bw_ui.h"
#include "bw_keys.h"
#include "fb_proto.h"
#include "fb_link.h"
#include "fb_kanji.h"
#include "fb_pics_a.h"
#include "fb_pics_b.h"
#include "fb_pics_c.h"
#include "fb_pics_d.h"
#include "fb_screen.h"

#define FB_HELLO_DELAY_MS 300u              /* after USB comes up: the host opens its MIDI port first */

/* the audio interrupt's (the main loop only writes the gain's target) */
static volatile int32_t fb_gain_target;     /* Q15, from MASTER */
static int32_t fb_gain;                     /* Q15: ramped to the target over a block */
static int16_t fb_in[2 * FW_BLOCK];

/* MASTER (0..1023) as a gain, Q15: squared, so the knob's travel is near even in dB; the bottom is silence */
static int32_t master_gain(uint32_t m)
{
    return m < 4u ? 0 : (int32_t)(m * m >> 5);
}

/* ---- the FM-1's own settings (EDIT), kept in storage object 0 */
#define FB_SET_MAGIC 0x4642u                /* "FB" */
#define FB_SET_LAYOUT 1u

typedef struct {
    uint16_t magic;
    uint8_t layout, ch, vel, manual;
} fb_set_t;

static int32_t fb_manual;                   /* Steckfeld never opens its programmer while connected */
static uint32_t fb_theme_sent;              /* the theme (system settings, fw_theme) the last HELLO carried */
static int fb_setup_open;
static fb_set_t fb_set_saved;

static void fb_set_get(fb_set_t* s)
{
    memset(s, 0, sizeof *s);
    s->magic = FB_SET_MAGIC;
    s->layout = FB_SET_LAYOUT;
    s->ch = (uint8_t)key_ch;
    s->vel = (uint8_t)key_vel;
    s->manual = (uint8_t)fb_manual;
}

static void fb_set_load(void)
{
    fb_set_t s;
    if (fw_store_load(0, &s, sizeof s) == (int)sizeof s && s.magic == FB_SET_MAGIC && s.layout == FB_SET_LAYOUT &&
        s.ch >= 1u && s.ch <= 16u && s.vel >= 1u && s.vel <= 127u && s.manual <= 1u) {
        key_ch = s.ch;
        key_vel = s.vel;
        fb_manual = s.manual;
    }
    fb_set_get(&fb_set_saved);
}

static int fb_usb_up(void) { return fw_stats.usb_up != 0; }

static void fb_hello(void)
{
    fb_theme_sent = fw_theme();
    if (fb_usb_up())
        fb_send_hello(fb_manual ? 1u : 0u, fb_theme_sent);
}

/* leaving the page: a change goes to the flash (the audio stalls a moment) and to the app */
static void fb_setup_close(void)
{
    fb_set_t s;
    fb_setup_open = 0;
    fb_set_get(&s);
    if (!memcmp(&s, &fb_set_saved, sizeof s))
        return;
    if (fw_store_save(0, &s, sizeof s) == 0)
        fb_set_saved = s;
    fb_hello();
}

static void fmt_open(char* b, int32_t v, const ui_param_t* p)
{
    (void)p;
    str_cpy(b, v ? "Never" : "Always", 8);
}

static const ui_param_t P_CH = { "MIDI CHANNEL", 1, 16, 1, &key_ch, 0, 0, 0 };
static const ui_param_t P_VEL = { "VELOCITY", 1, 127, 1, &key_vel, 0, 0, 0 };
static const ui_param_t P_OPEN = { "OPEN", 0, 1, 1, &fb_manual, fmt_open, 0, 0 };
static const ui_param_t* const FB_SETUP[3] = { &P_CH, &P_VEL, &P_OPEN };   /* (the theme: the system settings) */

/* ---- the controls */
static const uint32_t FB_KNOB[4] = { FW_KNOB_1, FW_KNOB_2, FW_KNOB_3, FW_KNOB_4 };
static const uint32_t FB_CAT_BTN[FB_CAT_COUNT] = { FW_BTN_HOME, FW_BTN_SCL, FW_BTN_FX, FW_BTN_LFO,
                                                   FW_BTN_ENV, FW_BTN_SEQ, FW_BTN_ARP, FW_BTN_GLO };
static int32_t fb_playing;                  /* the app's transport (SCREEN) */
static uint32_t fb_cat_spoilt;              /* category buttons (bits by category) that turned SELECT or closed EDIT:
                                               their release is no tap */
static uint32_t fb_btn_sent;                /* REC / SAVE whose press went out: their release goes out too */

static void cc(uint32_t num, uint32_t val)
{
    fw_midi_send((uint8_t)(0xB0u | (uint32_t)(key_ch - 1)), (uint8_t)num, (uint8_t)val);
}

/* an encoder's detents as one relative CC */
static void cc_rel(uint32_t num, int32_t detents)
{
    if (!detents)
        return;
    detents = detents > 63 ? 63 : detents < -63 ? -63 : detents;
    cc(num, (uint32_t)detents & 127u);
}

/* ---- the knobs from TIMER5 (APP_TICK), once a ms: their CCs go out as they turn, not once per drawn frame (a draw
   takes up to ~40 ms: a fast turn came as a few big jumps, and the app's acceleration saw it slow). Not on the
   settings page, whose knobs edit it in the main loop; what turned between two ms goes out from there too */
static volatile uint8_t fb_tick_on;                       /* main loop: not the settings page */
static volatile uint8_t fb_tick_turned;                   /* ALGORITHM or KNOB1..4 turned: the patch list closes */
static volatile uint8_t fb_tick_spoil[FB_CAT_COUNT];      /* SELECT turned with this category held: no tap */

static int32_t knob_tick(uint32_t knob, uint32_t num)
{
    int32_t d = fw_knob_take_tick(knob);
    if (!d)
        return 0;
    d = d > 63 ? 63 : d < -63 ? -63 : d;
    fw_midi_send_tick((uint8_t)(0xB0u | (uint32_t)(key_ch - 1)), (uint8_t)num, (uint8_t)((uint32_t)d & 127u));
    return d;
}

static void app_tick(void)
{
    static uint32_t sub;
    uint32_t i, held, cat = FB_CAT_COUNT;
    if (++sub < 10u)
        return;
    sub = 0;
    if (!fb_tick_on)
        return;
    held = fw_held_tick();
    for (i = 0; i < FB_CAT_COUNT && cat == FB_CAT_COUNT; i++)
        if (held & FW_BIT(FB_CAT_BTN[i]))
            cat = i;
    if (cat < FB_CAT_COUNT) {
        if (knob_tick(FW_KNOB_SELECT, FB_CC_CAT_TURN + cat))
            fb_tick_spoil[cat] = 1;
    } else {
        (void)knob_tick(FW_KNOB_SELECT, FB_CC_SELECT);
    }
    (void)knob_tick(FW_KNOB_PRESET, FB_CC_PRESETS);
    if (knob_tick(FW_KNOB_ALGO, FB_CC_ALGO))
        fb_tick_turned = 1;
    for (i = 0; i < 4u; i++)
        if (knob_tick(FB_KNOB[i], FB_CC_KNOB + i))
            fb_tick_turned = 1;
}

static void cc_button(const fw_input_t* in, uint32_t btn, uint32_t num, int local)
{
    if (in->pressed & FW_BIT(btn) && !local) {
        cc(num, 127u);
        fb_btn_sent |= FW_BIT(btn);
    }
    if (in->released & FW_BIT(btn) && fb_btn_sent & FW_BIT(btn)) {
        cc(num, 0);
        fb_btn_sent &= ~FW_BIT(btn);
    }
}

/* the settings page: the knobs edit it, EDIT or a category button leaves */
static void fb_setup_controls(const fw_input_t* in)
{
    uint32_t i;
    for (i = 0; i < FW_NELEM(FB_SETUP); i++)
        ui_turn(FB_SETUP[i], in->knob[FB_KNOB[i]]);
    for (i = 0; i < FB_CAT_COUNT; i++)
        if (in->pressed & FW_BIT(FB_CAT_BTN[i])) {
            fb_cat_spoilt |= FW_BIT(i);
            fb_setup_close();
        }
}

static void controls(const fw_input_t* in)
{
    uint32_t i, held_cat = FB_CAT_COUNT;
    int local;
    if (in->pressed & FW_BIT(FW_BTN_EDIT)) {
        if (fb_setup_open)
            fb_setup_close();
        else
            fb_setup_open = 1;
    }
    local = fb_setup_open;
    fb_tick_on = (uint8_t)!local;
    if (local)
        fb_setup_controls(in);
    for (i = 0; i < FB_CAT_COUNT; i++) {
        const uint32_t b = FW_BIT(FB_CAT_BTN[i]);
        if (in->pressed & b)
            fb_cat_spoilt &= ~FW_BIT(i) | (local ? FW_BIT(i) : 0u);
        if (in->held & b && held_cat == FB_CAT_COUNT)
            held_cat = i;
        if (fb_tick_spoil[i]) {
            fb_tick_spoil[i] = 0;
            fb_cat_spoilt |= FW_BIT(i);
        }
    }
    if (!local) {
        if (held_cat < FB_CAT_COUNT && in->knob[FW_KNOB_SELECT]) {   /* category held + SELECT: its modules */
            cc_rel(FB_CC_CAT_TURN + held_cat, in->knob[FW_KNOB_SELECT]);
            fb_cat_spoilt |= FW_BIT(held_cat);
        } else {
            cc_rel(FB_CC_SELECT, in->knob[FW_KNOB_SELECT]);
        }
        cc_rel(FB_CC_ALGO, in->knob[FW_KNOB_ALGO]);
        cc_rel(FB_CC_PRESETS, in->knob[FW_KNOB_PRESET]);
        for (i = 0; i < 4u; i++)
            cc_rel(FB_CC_KNOB + i, in->knob[FB_KNOB[i]]);
        /* the patch list closes on any control but PRESETS, SELECT (its filter) and REC (the app closes it too; here at once) */
        if (fb.list[1].open && (fb_tick_turned || in->knob[FW_KNOB_ALGO] || in->knob[FB_KNOB[0]] ||
                                in->knob[FB_KNOB[1]] || in->knob[FB_KNOB[2]] || in->knob[FB_KNOB[3]] ||
                                (in->pressed & ~(FW_BIT(FW_BTN_REC) | FW_BIT(FW_BTN_OCT_DOWN) | FW_BIT(FW_BTN_OCT_UP)))))
            fb_list_close();
        fb_tick_turned = 0;
    }
    for (i = 0; i < FB_CAT_COUNT; i++)
        if (in->released & FW_BIT(FB_CAT_BTN[i]) && !(fb_cat_spoilt & FW_BIT(i)) && !local)
            cc(FB_CC_CAT_TAP + i, 127u);
    cc_button(in, FW_BTN_REC, FB_CC_REC, local);
    cc_button(in, FW_BTN_SAVE, FB_CC_SAVE, local);
    cc_button(in, FW_BTN_PLAY, FB_CC_PLAY, local);
}

static void leds(void)
{
    uint32_t i;
    const int on = fb.link == FB_LINK_ON && !fb_setup_open;
    uint32_t dark = 0;                      /* categories with no module: no backlight */
    for (i = 0; i < FB_CAT_COUNT; i++) {
        fw_led_button(FB_CAT_BTN[i], on && fb.scr.cat == i);
        if (fb.link == FB_LINK_ON && !(fb.bay & FW_BIT(i)))
            dark |= FW_BIT(FB_CAT_BTN[i]);
    }
    fw_lights_off(dark);
    fw_led_button(FW_BTN_EDIT, fb_setup_open);
    fw_led_button(FW_BTN_PLAY, fb_playing);
}

/* ---- the screen: what each region shows now, drawn where it changed */
enum { FB_V_WAIT, FB_V_OLD, FB_V_MAIN, FB_V_LIST, FB_V_SETUP };

static struct {
    uint32_t view, dark, pic_serial, pic_ms, pic_key;
    fb_head_t head;
    fb_ctl_t tile[4];
    fb_foot_t foot;
} shown;
static int fb_never_drawn = 1;

static uint32_t fb_view(void)
{
    if (fb_setup_open)
        return FB_V_SETUP;
    if (fb.link == FB_LINK_ON)
        return fb.list[1].open ? FB_V_LIST : FB_V_MAIN;
    return fb.link == FB_LINK_WAIT ? FB_V_WAIT : FB_V_OLD;
}

static const char* fb_audio_text(void)
{
    const int s = usb_play_state();
    return s == 2 ? "On" : s == 1 ? "Waiting" : "Off";
}

static void fb_local_tile(fb_ctl_t* c, const char* title, const char* readout, int32_t v, int32_t lo, int32_t hi)
{
    memset(c, 0, sizeof *c);
    c->flags = (uint8_t)(FB_CTL_PRESENT | (hi > lo ? 0u : FB_CTL_NOBAR));
    c->pos = (uint16_t)(hi > lo ? (uint32_t)(v - lo) * FB_V_MAX / (uint32_t)(hi - lo) : 0u);
    str_cpy(c->title, title, sizeof c->title);
    str_cpy(c->readout, readout, sizeof c->readout);
}

static void fb_want_tiles(uint32_t view, fb_ctl_t* t)
{
    char b[16];
    uint32_t i;
    if (view == FB_V_MAIN) {
        memcpy(t, fb.ctl, 4u * sizeof *t);
        return;
    }
    if (view == FB_V_SETUP) {
        memset(t, 0, 4u * sizeof *t);
        for (i = 0; i < FW_NELEM(FB_SETUP); i++) {
            ui_value(b, FB_SETUP[i]);
            fb_local_tile(&t[i], FB_SETUP[i]->name, b, *FB_SETUP[i]->val, FB_SETUP[i]->min, FB_SETUP[i]->max);
        }
        return;
    }
    fmt_int(b, key_ch);
    fb_local_tile(&t[0], "MIDI CHANNEL", b, 0, 0, 0);
    b[0] = 0;
    if (key_oct > 0)
        ui_cat(b, "+", sizeof b);
    ui_cat_int(b, key_oct, sizeof b);
    fb_local_tile(&t[1], "OCTAVE", b, 0, 0, 0);
    fmt_int(b, key_vel);
    fb_local_tile(&t[2], "VELOCITY", b, 0, 0, 0);
    fb_local_tile(&t[3], "USB AUDIO", fb_audio_text(), 0, 0, 0);
}

static void fb_want_head(uint32_t view, fb_head_t* h)
{
    memset(h, 0, sizeof *h);
    if (view == FB_V_SETUP) {
        str_cpy(h->title, "FM-1 SETTINGS", sizeof h->title);
        return;
    }
    if (view == FB_V_WAIT || view == FB_V_OLD) {
        str_cpy(h->title, view == FB_V_OLD ? "UPDATE NEEDED" : "FERNBEDIENUNG", sizeof h->title);
        return;
    }
    str_cpy(h->title, fb.scr.title[0] ? fb.scr.title : "STECKFELD", sizeof h->title);
    if (view == FB_V_LIST) {
        str_cpy(h->title, "PATCHES", sizeof h->title);
        if (fb.list[1].title[0]) {
            ui_cat(h->title, " / ", sizeof h->title);
            ui_cat(h->title, fb.list[1].title, sizeof h->title);
        }
        return;
    }
    h->route = fb.scr.mode == 1u;
    h->tag = h->route ? 2u : 1u;
    h->pages = fb.scr.pages;
    h->cur = fb.scr.page;
    if (fb.scr.page < fb.scr.pages)
        str_cpy(h->page, fb.scr.page_title[fb.scr.page], sizeof h->page);
    h->unsaved = (uint8_t)(fb.scr.flags & FB_SCR_UNSAVED);
}

static void fb_want_foot(uint32_t view, fb_foot_t* f)
{
    memset(f, 0, sizeof *f);
    if (view == FB_V_SETUP) {
        str_cpy(f->l_chip, "EDIT", sizeof f->l_chip);
        str_cpy(f->l_text, "back", sizeof f->l_text);
        str_cpy(f->r_text, FW_VERSION, sizeof f->r_text);
        return;
    }
    if (view == FB_V_WAIT || view == FB_V_OLD) {   /* (the tiles show the channel and the USB audio) */
        str_cpy(f->l_chip, "EDIT", sizeof f->l_chip);
        str_cpy(f->l_text, "settings", sizeof f->l_text);
        return;
    }
    if (view == FB_V_LIST) {
        str_cpy(f->l_chip, "REC", sizeof f->l_chip);
        str_cpy(f->l_text, "load", sizeof f->l_text);
        str_cpy(f->r_chip, "PRESETS", sizeof f->r_chip);
        str_cpy(f->r_text, "browse", sizeof f->r_text);
        return;
    }
    if (view == FB_V_MAIN && fb.notice.text[0]) {   /* a notice first: REC acts on it */
        str_cpy(f->l_text, fb.notice.text, sizeof f->l_text);
        f->notice = 1;
        if (fb.notice.act[0]) {
            str_cpy(f->r_chip, "REC", sizeof f->r_chip);
            str_cpy(f->r_text, fb.notice.act, sizeof f->r_text);
        }
        return;
    }
    if (view == FB_V_MAIN && (fb.scr.rec[0] || fb.scr.save[0])) {
        if (fb.scr.rec[0]) {
            str_cpy(f->l_chip, "REC", sizeof f->l_chip);
            str_cpy(f->l_text, fb.scr.rec, sizeof f->l_text);
        }
        if (fb.scr.save[0]) {
            str_cpy(f->r_chip, "SAVE", sizeof f->r_chip);
            str_cpy(f->r_text, fb.scr.save, sizeof f->r_text);
        }
        return;
    }
    str_cpy(f->l_text, "MIDI ch ", sizeof f->l_text);
    ui_cat_int(f->l_text, key_ch, sizeof f->l_text);
    ui_cat(f->l_text, "   USB audio ", sizeof f->l_text);
    ui_cat(f->l_text, fb_audio_text(), sizeof f->l_text);
}

/* the waiting, update and settings pages' picture: two lines of text */
static void fb_local_pic(uint32_t view)
{
    char b[48];
    if (view == FB_V_SETUP) {
        fb_pic_message("Knobs 1-4 change these", "Saved when you leave this page");
    } else if (view == FB_V_OLD) {
        str_cpy(b, "Protocol: Steckfeld ", sizeof b);
        ui_cat_int(b, (int32_t)fb.app_version, sizeof b);
        ui_cat(b, ", FM-1 ", sizeof b);
        ui_cat_int(b, FB_PROTO_VERSION, sizeof b);
        fb_pic_message(fb.link == FB_LINK_OLD_FM1 ? "Update the FM-1" : "Update Steckfeld SF1", b);
    } else if (!fb_usb_up()) {
        fb_pic_message("No USB connection", "Plug the FM-1 into the iPad");
    } else {
        fb_pic_message("Steckfeld not connected", "Open Steckfeld SF1 on the iPad");
    }
}

static void draw(uint32_t ms)
{
    const uint32_t view = fb_view();
    const uint32_t crew = view == FB_V_MAIN && fb.scr.mode == 0u ? pc_crew_of(fb.scr.kind) : 0u;
    const uint32_t dark = fw_theme() == FW_THEME_DARK ||
                          (fw_theme() == FW_THEME_SYNC && fb.link == FB_LINK_ON && fb.app_theme == 2u);
    fb_head_t h;
    fb_ctl_t t[4];
    fb_foot_t f;
    uint32_t i, dirty = fb.dirty, key;
    fb.dirty = 0;
    if (fb_never_drawn || view != shown.view || dark != shown.dark || crew != fb_crew) {
        fb_never_drawn = 0;
        fb_crew = crew;
        fb_theme_set(dark);
        shown.view = view;
        shown.dark = dark;
        dirty = FB_D_ALL;
        fb_gaps();
    }
    fb_want_head(view, &h);
    if (dirty & FB_D_HEAD || memcmp(&h, &shown.head, sizeof h)) {
        fb_head_draw(&h);
        shown.head = h;
    }
    if (view == FB_V_LIST) {
        const int full = (dirty & (FB_D_PIC | FB_D_TILES)) != 0;
        if (full)
            fb_overlay_draw(&fb.list[1]);
        fb_overlay_scroll(&fb.list[1], ms, full);
    } else {
        if (view == FB_V_MAIN) {
            const int moves = fb.scr.mode == 0u &&
                              (fb_pics_a_moves(&fb.pic) || fb_pics_c_moves(&fb.pic) || fb_pics_d_moves(&fb.pic));
            key = fb.scr.mode;
            if (dirty & FB_D_PIC || key != shown.pic_key ||
                ((fb.pic.serial != shown.pic_serial || moves) && ms - shown.pic_ms >= FB_PIC_MS)) {
                if (fb.scr.mode == 1u)
                    fb_route_draw(&fb.list[0]);
                else
                    fb_pic_draw(&fb.pic);
                shown.pic_serial = fb.pic.serial;
                shown.pic_ms = ms;
            }
        } else {
            key = (uint32_t)fb_usb_up() | (uint32_t)(fb.link == FB_LINK_ON) << 1;
            if (dirty & FB_D_PIC || key != shown.pic_key)
                fb_local_pic(view);
        }
        shown.pic_key = key;
        fb_want_tiles(view, t);
        for (i = 0; i < 4u; i++)
            if (dirty & (4u << i) || memcmp(&t[i], &shown.tile[i], sizeof t[i])) {
                fb_tile_draw(i, &t[i]);
                shown.tile[i] = t[i];
            }
    }
    fb_want_foot(view, &f);
    if (dirty & FB_D_FOOT || memcmp(&f, &shown.foot, sizeof f)) {
        fb_foot_draw(&f);
        shown.foot = f;
    }
}

/* ---- the platform's hooks */
static void app_init(void)
{
    keys_init();
    fb_set_load();
    fb_theme_set(fw_theme() == FW_THEME_DARK);
    fb.pic.kind = 0xFFu;
    usb_play_enable(1);
}

/* a SysEx frame from the host (sys/main.c, the main loop: never inside app_frame) */
static void app_sysex(const uint8_t* p, uint32_t n)
{
    if (fb_take(p, n))
        fb_hello();
}

static void app_frame(const fw_input_t* in)
{
    static uint32_t up_ms, hello_due, running = 2u;
    static int was_up;
    const int up = fb_usb_up();
    fw_midi_t m;
    fb_gain_target = master_gain(in->master);
    if (in->redraw) {
        fb_never_drawn = 1;
        if (fw_theme() != fb_theme_sent)    /* the theme changed in the system settings: the app hears it */
            fb_hello();
    }
    while (fw_midi_read(&m))                /* (drained: a full ring would hold the host's SysEx back too) */
        ;
    if (up && !was_up) {                    /* USB came up: say hello once, a moment later */
        up_ms = in->ms;
        hello_due = 1;
    }
    if (!up && was_up && fb.link != FB_LINK_WAIT) {
        fb.link = FB_LINK_WAIT;
        fb_list_close();
    }
    was_up = up;
    if (hello_due && up && in->ms - up_ms >= FB_HELLO_DELAY_MS) {
        hello_due = 0;
        fb_hello();
    }
    if (fb.link == FB_LINK_ON && (fb.scr.flags & FB_SCR_RUNNING) != running) {   /* the app's transport, when it changes */
        running = fb.scr.flags & FB_SCR_RUNNING;
        fb_playing = running != 0;
    }
    keys_frame(in);
    controls(in);
    leds();
    draw(in->ms);
}

/* pair 1 of the USB playback (or silence), at MASTER's volume */
static void app_audio(int32_t* lr, uint32_t frames)
{
    const int32_t g1 = fb_gain_target, step = (g1 - fb_gain) / (int32_t)frames;
    const int on = usb_play_begin() && usb_play_read(fb_in, frames);
    int32_t g = fb_gain;
    uint32_t i;
    for (i = 0; i < frames; i++) {
        g += step;
        lr[2u * i] = on ? fb_in[2u * i] * g >> 15 : 0;
        lr[2u * i + 1u] = on ? fb_in[2u * i + 1u] * g >> 15 : 0;
    }
    fb_gain = g1;
}
