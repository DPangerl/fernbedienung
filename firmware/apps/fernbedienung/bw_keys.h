/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Daniel Pangerl */
/* Beiwagen's keys as a MIDI keyboard (unity build, included once by app.c): the 27 keys send notes over USB-MIDI to
 * the host (the OP-1), OCT- / OCT+ shift them by octaves. A key's note-off goes out on the note and channel its
 * note-on did, whatever changed while it was held. */
#ifndef BW_KEYS_H
#define BW_KEYS_H

#define KEY_NONE 0xFFu

static int32_t key_oct;                     /* -3..3 */
static int32_t key_ch = 1;                  /* 1..16 */
static int32_t key_vel = 100;               /* 1..127 */
static uint8_t key_note[FW_KEYS], key_chan[FW_KEYS];

static void keys_init(void)
{
    memset(key_note, KEY_NONE, sizeof key_note);
}

static void keys_off(uint32_t k)
{
    if (key_note[k] == KEY_NONE)
        return;
    fw_midi_send((uint8_t)(0x80u | key_chan[k]), key_note[k], 0);
    key_note[k] = KEY_NONE;
}

static void keys_frame(const fw_input_t* in)
{
    uint32_t k;
    if (in->pressed & FW_BIT(FW_BTN_OCT_DOWN) && key_oct > -3)
        key_oct--;
    if (in->pressed & FW_BIT(FW_BTN_OCT_UP) && key_oct < 3)
        key_oct++;
    for (k = 0; k < FW_KEYS; k++) {
        const uint32_t bit = FW_BIT(k);
        if (in->keys_released & bit)
            keys_off(k);
        if (in->keys_pressed & bit) {
            const int32_t n = (int32_t)FW_KEY_BASE_NOTE + (int32_t)k + 12 * key_oct;
            keys_off(k);
            if (n < 0 || n > 127)
                continue;
            key_note[k] = (uint8_t)n;
            key_chan[k] = (uint8_t)(key_ch - 1);
            fw_midi_send((uint8_t)(0x90u | key_chan[k]), key_note[k], (uint8_t)key_vel);
        }
    }
    fw_leds_keys(in->keys);
    fw_led_button(FW_BTN_OCT_DOWN, key_oct < 0);
    fw_led_button(FW_BTN_OCT_UP, key_oct > 0);
}

#endif
