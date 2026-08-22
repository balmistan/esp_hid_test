/*
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef HID_CONSUMER_H
#define HID_CONSUMER_H

#include <stdint.h>
#include <stdbool.h>

#include "esp_hidd.h"


/*
 * ============================================================
 * CONSUMER CONTROL
 * ============================================================
 */

#define HID_CONSUMER_POWER          48
#define HID_CONSUMER_RESET          49
#define HID_CONSUMER_SLEEP          50

#define HID_CONSUMER_MENU           64
#define HID_CONSUMER_SELECTION      128
#define HID_CONSUMER_ASSIGN_SEL     129
#define HID_CONSUMER_MODE_STEP      130
#define HID_CONSUMER_RECALL_LAST    131
#define HID_CONSUMER_QUIT           148
#define HID_CONSUMER_HELP           149
#define HID_CONSUMER_CHANNEL_UP     156
#define HID_CONSUMER_CHANNEL_DOWN   157

#define HID_CONSUMER_PLAY           176
#define HID_CONSUMER_PAUSE          177
#define HID_CONSUMER_RECORD         178
#define HID_CONSUMER_FAST_FORWARD   179
#define HID_CONSUMER_REWIND         180
#define HID_CONSUMER_SCAN_NEXT_TRK  181
#define HID_CONSUMER_SCAN_PREV_TRK  182
#define HID_CONSUMER_STOP           183
#define HID_CONSUMER_EJECT          184
#define HID_CONSUMER_RANDOM_PLAY    185
#define HID_CONSUMER_SELECT_DISC    186
#define HID_CONSUMER_ENTER_DISC     187
#define HID_CONSUMER_REPEAT         188
#define HID_CONSUMER_STOP_EJECT     204
#define HID_CONSUMER_PLAY_PAUSE     205
#define HID_CONSUMER_PLAY_SKIP      206

#define HID_CONSUMER_VOLUME         224
#define HID_CONSUMER_BALANCE        225
#define HID_CONSUMER_MUTE           226
#define HID_CONSUMER_BASS           227
#define HID_CONSUMER_VOLUME_UP      233
#define HID_CONSUMER_VOLUME_DOWN    234


/*
 * ============================================================
 * API
 * ============================================================
 */

void hid_consumer_init(
    esp_hidd_dev_t *hid_dev
);

void esp_hidd_send_consumer_value(
    uint8_t key_cmd,
    bool key_pressed
);

#endif