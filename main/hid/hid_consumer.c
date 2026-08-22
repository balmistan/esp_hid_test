/*
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "hid_consumer.h"


/*
 * ============================================================
 * LOCAL HID DEVICE
 * ============================================================
 */

static esp_hidd_dev_t *s_hid_dev = NULL;


/*
 * ============================================================
 * REPORT DEFINITIONS
 * ============================================================
 */

#define HID_CC_RPT_MUTE                 1
#define HID_CC_RPT_POWER                2
#define HID_CC_RPT_LAST                 3
#define HID_CC_RPT_ASSIGN_SEL           4
#define HID_CC_RPT_PLAY                 5
#define HID_CC_RPT_PAUSE                6
#define HID_CC_RPT_RECORD               7
#define HID_CC_RPT_FAST_FWD             8
#define HID_CC_RPT_REWIND               9
#define HID_CC_RPT_SCAN_NEXT_TRK        10
#define HID_CC_RPT_SCAN_PREV_TRK        11
#define HID_CC_RPT_STOP                 12

#define HID_CC_RPT_CHANNEL_UP           0x10
#define HID_CC_RPT_CHANNEL_DOWN         0x30
#define HID_CC_RPT_VOLUME_UP            0x40
#define HID_CC_RPT_VOLUME_DOWN          0x80

#define HID_CC_RPT_NUMERIC_BITS         0xF0
#define HID_CC_RPT_CHANNEL_BITS         0xCF
#define HID_CC_RPT_VOLUME_BITS          0x3F
#define HID_CC_RPT_BUTTON_BITS          0xF0
#define HID_CC_RPT_SELECTION_BITS       0xCF


#define HID_CC_RPT_SET_NUMERIC(s, x) \
    (s)[0] &= HID_CC_RPT_NUMERIC_BITS; \
    (s)[0] = (x)

#define HID_CC_RPT_SET_CHANNEL(s, x) \
    (s)[0] &= HID_CC_RPT_CHANNEL_BITS; \
    (s)[0] |= ((x) & 0x03) << 4

#define HID_CC_RPT_SET_VOLUME_UP(s) \
    (s)[0] &= HID_CC_RPT_VOLUME_BITS; \
    (s)[0] |= 0x40

#define HID_CC_RPT_SET_VOLUME_DOWN(s) \
    (s)[0] &= HID_CC_RPT_VOLUME_BITS; \
    (s)[0] |= 0x80

#define HID_CC_RPT_SET_BUTTON(s, x) \
    (s)[1] &= HID_CC_RPT_BUTTON_BITS; \
    (s)[1] |= (x)

#define HID_CC_RPT_SET_SELECTION(s, x) \
    (s)[1] &= HID_CC_RPT_SELECTION_BITS; \
    (s)[1] |= ((x) & 0x03) << 4


/*
 * ============================================================
 * REPORT
 * ============================================================
 */

#define HID_RPT_ID_CC_IN       3
#define HID_CC_IN_RPT_LEN      2


/*
 * ============================================================
 * INITIALIZATION
 * ============================================================
 */

void hid_consumer_init(
    esp_hidd_dev_t *hid_dev)
{
    s_hid_dev = hid_dev;
}


/*
 * ============================================================
 * SEND CONSUMER VALUE
 * ============================================================
 */

void esp_hidd_send_consumer_value(
    uint8_t key_cmd,
    bool key_pressed)
{
    uint8_t buffer[HID_CC_IN_RPT_LEN] = {
        0,
        0
    };


    if (!s_hid_dev) {
        return;
    }


    if (key_pressed) {

        switch (key_cmd) {

        case HID_CONSUMER_CHANNEL_UP:

            HID_CC_RPT_SET_CHANNEL(
                buffer,
                HID_CC_RPT_CHANNEL_UP
            );

            break;


        case HID_CONSUMER_CHANNEL_DOWN:

            HID_CC_RPT_SET_CHANNEL(
                buffer,
                HID_CC_RPT_CHANNEL_DOWN
            );

            break;


        case HID_CONSUMER_VOLUME_UP:

            HID_CC_RPT_SET_VOLUME_UP(
                buffer
            );

            break;


        case HID_CONSUMER_VOLUME_DOWN:

            HID_CC_RPT_SET_VOLUME_DOWN(
                buffer
            );

            break;


        case HID_CONSUMER_MUTE:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_MUTE
            );

            break;


        case HID_CONSUMER_POWER:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_POWER
            );

            break;


        case HID_CONSUMER_RECALL_LAST:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_LAST
            );

            break;


        case HID_CONSUMER_ASSIGN_SEL:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_ASSIGN_SEL
            );

            break;


        case HID_CONSUMER_PLAY:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_PLAY
            );

            break;


        case HID_CONSUMER_PAUSE:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_PAUSE
            );

            break;


        case HID_CONSUMER_RECORD:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_RECORD
            );

            break;


        case HID_CONSUMER_FAST_FORWARD:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_FAST_FWD
            );

            break;


        case HID_CONSUMER_REWIND:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_REWIND
            );

            break;


        case HID_CONSUMER_SCAN_NEXT_TRK:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_SCAN_NEXT_TRK
            );

            break;


        case HID_CONSUMER_SCAN_PREV_TRK:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_SCAN_PREV_TRK
            );

            break;


        case HID_CONSUMER_STOP:

            HID_CC_RPT_SET_BUTTON(
                buffer,
                HID_CC_RPT_STOP
            );

            break;


        default:

            break;
        }
    }


    esp_hidd_dev_input_set(
        s_hid_dev,
        0,
        HID_RPT_ID_CC_IN,
        buffer,
        HID_CC_IN_RPT_LEN
    );
}