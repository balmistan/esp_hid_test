#include "hid_keyboard.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"


static const char *TAG = "HID_KEYBOARD";


/*
 * HID device used by this module.
 */
static esp_hidd_dev_t *s_hid_dev = NULL;


/*
 * ============================================================
 * BLE HID KEYBOARD + MOUSE REPORT MAP
 * ============================================================
 *
 * Report ID 1 = Keyboard
 * Report ID 2 = Mouse
 *
 * This combined report map is used by:
 *
 *     CONFIG_EXAMPLE_HID_DEVICE_ROLE == 2
 *
 * The keyboard and mouse therefore belong to the
 * same BLE HID device.
 */

const unsigned char keyboardMouseReportMap[] = {

    /*
     * ========================================================
     * KEYBOARD
     * Report ID 1
     * ========================================================
     */

    0x05, 0x01,             /* Usage Page (Generic Desktop) */
    0x09, 0x06,             /* Usage (Keyboard) */
    0xA1, 0x01,             /* Collection (Application) */

    0x85, 0x01,             /* Report ID (1) */

    0x05, 0x07,             /* Usage Page (Keyboard/Keypad) */

    0x19, 0xE0,             /* Usage Minimum (Keyboard LeftControl) */
    0x29, 0xE7,             /* Usage Maximum (Keyboard Right GUI) */

    0x15, 0x00,             /* Logical Minimum (0) */
    0x25, 0x01,             /* Logical Maximum (1) */

    0x75, 0x01,             /* Report Size (1) */
    0x95, 0x08,             /* Report Count (8) */

    0x81, 0x02,             /* Input (Data, Variable, Absolute) */

    0x95, 0x01,             /* Report Count (1) */
    0x75, 0x08,             /* Report Size (8) */

    0x81, 0x03,             /* Input (Constant) */

    0x95, 0x05,             /* Report Count (5) */
    0x75, 0x01,             /* Report Size (1) */

    0x05, 0x08,             /* Usage Page (LEDs) */

    0x19, 0x01,             /* Usage Minimum (Num Lock) */
    0x29, 0x05,             /* Usage Maximum (Kana) */

    0x91, 0x02,             /* Output (Data, Variable, Absolute) */

    0x95, 0x01,             /* Report Count (1) */
    0x75, 0x03,             /* Report Size (3) */

    0x91, 0x03,             /* Output (Constant) */

    0x95, 0x05,             /* Report Count (5) */
    0x75, 0x08,             /* Report Size (8) */

    0x15, 0x00,             /* Logical Minimum (0) */
    0x25, 0x65,             /* Logical Maximum (101) */

    0x05, 0x07,             /* Usage Page (Keyboard/Keypad) */

    0x19, 0x00,             /* Usage Minimum (Reserved) */
    0x29, 0x65,             /* Usage Maximum (Keyboard Application) */

    0x81, 0x00,             /* Input (Data, Array, Absolute) */

    0xC0,                   /* End Collection */


    /*
     * ========================================================
     * MOUSE
     * Report ID 2
     * ========================================================
     */

    0x05, 0x01,             /* Usage Page (Generic Desktop) */
    0x09, 0x02,             /* Usage (Mouse) */
    0xA1, 0x01,             /* Collection (Application) */

    /*
     * IMPORTANT:
     *
     * The mouse uses Report ID 2.
     */
    0x85, 0x02,             /* Report ID (2) */

    0x09, 0x01,             /* Usage (Pointer) */
    0xA1, 0x00,             /* Collection (Physical) */

    /*
     * --------------------------------------------------------
     * Mouse buttons
     * --------------------------------------------------------
     */

    0x05, 0x09,             /* Usage Page (Button) */

    0x19, 0x01,             /* Usage Minimum (Button 1) */
    0x29, 0x03,             /* Usage Maximum (Button 3) */

    0x15, 0x00,             /* Logical Minimum (0) */
    0x25, 0x01,             /* Logical Maximum (1) */

    0x95, 0x03,             /* Report Count (3) */
    0x75, 0x01,             /* Report Size (1) */

    0x81, 0x02,             /* Input (Data, Variable, Absolute) */

    /*
     * Padding for mouse buttons.
     */

    0x95, 0x01,             /* Report Count (1) */
    0x75, 0x05,             /* Report Size (5) */

    0x81, 0x03,             /* Input (Constant) */

    /*
     * --------------------------------------------------------
     * Mouse X / Y / Wheel
     * --------------------------------------------------------
     */

    0x05, 0x01,             /* Usage Page (Generic Desktop) */

    0x09, 0x30,             /* Usage (X) */
    0x09, 0x31,             /* Usage (Y) */
    0x09, 0x38,             /* Usage (Wheel) */

    0x15, 0x81,             /* Logical Minimum (-127) */
    0x25, 0x7F,             /* Logical Maximum (127) */

    0x75, 0x08,             /* Report Size (8) */
    0x95, 0x03,             /* Report Count (3) */

    0x81, 0x06,             /* Input (Data, Variable, Relative) */

    0xC0,                   /* End Physical Collection */
    0xC0                    /* End Application Collection */
};


/*
 * ============================================================
 * USB HID MODIFIER CODES
 * ============================================================
 */

#define USB_HID_MODIFIER_LEFT_CTRL      0x01
#define USB_HID_MODIFIER_LEFT_SHIFT     0x02
#define USB_HID_MODIFIER_LEFT_ALT       0x04

#define USB_HID_MODIFIER_RIGHT_CTRL     0x10
#define USB_HID_MODIFIER_RIGHT_SHIFT    0x20
#define USB_HID_MODIFIER_RIGHT_ALT      0x40


/*
 * ============================================================
 * USB HID KEY CODES
 * ============================================================
 */

#define USB_HID_SPACE                   0x2C
#define USB_HID_DOT                     0x37
#define USB_HID_NEWLINE                 0x28
#define USB_HID_FSLASH                  0x38
#define USB_HID_BSLASH                  0x31
#define USB_HID_COMMA                   0x36

#define USB_HID_BACKSPACE               0x2A


/*
 * ============================================================
 * ASCII -> HID HELPER
 * ============================================================
 */

#define CASE(a, b, c)       \
    case a:                 \
        buffer[0] = b;      \
        buffer[2] = c;      \
        break;


/*
 * ============================================================
 * ASCII -> HID
 * ============================================================
 */

static void char_to_code(
    uint8_t *buffer,
    char ch
)
{
    /*
     * Lowercase letters.
     */
    if (ch >= 'a' && ch <= 'z') {

        buffer[0] = 0;

        buffer[2] =
            (uint8_t)(4 + (ch - 'a'));
    }


    /*
     * Uppercase letters.
     */
    else if (ch >= 'A' && ch <= 'Z') {

        buffer[0] =
            USB_HID_MODIFIER_LEFT_SHIFT;

        ch =
            ch - ('A' - 'a');

        buffer[2] =
            (uint8_t)(4 + (ch - 'a'));
    }


    /*
     * Numbers.
     */
    else if (ch >= '0' && ch <= '9') {

        buffer[0] = 0;

        if (ch == '0') {

            buffer[2] = 39;

        } else {

            buffer[2] =
                (uint8_t)(30 + (ch - '1'));
        }
    }


    /*
     * Special characters.
     */
    else {

        switch (ch) {

        CASE(
            ' ',
            0,
            USB_HID_SPACE
        );

        CASE(
            '.',
            0,
            USB_HID_DOT
        );

        CASE(
            '\n',
            0,
            USB_HID_NEWLINE
        );

        CASE(
            '?',
            USB_HID_MODIFIER_LEFT_SHIFT,
            USB_HID_FSLASH
        );

        CASE(
            '/',
            0,
            USB_HID_FSLASH
        );

        CASE(
            '\\',
            0,
            USB_HID_BSLASH
        );

        CASE(
            '|',
            USB_HID_MODIFIER_LEFT_SHIFT,
            USB_HID_BSLASH
        );

        CASE(
            ',',
            0,
            USB_HID_COMMA
        );

        CASE(
            '<',
            USB_HID_MODIFIER_LEFT_SHIFT,
            USB_HID_COMMA
        );

        CASE(
            '>',
            USB_HID_MODIFIER_LEFT_SHIFT,
            USB_HID_COMMA
        );

        CASE(
            '@',
            USB_HID_MODIFIER_LEFT_SHIFT,
            31
        );

        CASE(
            '!',
            USB_HID_MODIFIER_LEFT_SHIFT,
            30
        );

        CASE(
            '#',
            USB_HID_MODIFIER_LEFT_SHIFT,
            32
        );

        CASE(
            '$',
            USB_HID_MODIFIER_LEFT_SHIFT,
            33
        );

        CASE(
            '%',
            USB_HID_MODIFIER_LEFT_SHIFT,
            34
        );

        CASE(
            '^',
            USB_HID_MODIFIER_LEFT_SHIFT,
            35
        );

        CASE(
            '&',
            USB_HID_MODIFIER_LEFT_SHIFT,
            36
        );

        CASE(
            '*',
            USB_HID_MODIFIER_LEFT_SHIFT,
            37
        );

        CASE(
            '(',
            USB_HID_MODIFIER_LEFT_SHIFT,
            38
        );

        CASE(
            ')',
            USB_HID_MODIFIER_LEFT_SHIFT,
            39
        );

        CASE(
            '-',
            0,
            0x2D
        );

        CASE(
            '_',
            USB_HID_MODIFIER_LEFT_SHIFT,
            0x2D
        );

        CASE(
            '=',
            0,
            0x2E
        );

        CASE(
            '+',
            USB_HID_MODIFIER_LEFT_SHIFT,
            39
        );

        CASE(
            8,
            0,
            USB_HID_BACKSPACE
        );

        CASE(
            '\t',
            0,
            0x2B
        );

        default:

            buffer[0] = 0;
            buffer[2] = 0;

            break;
        }
    }
}


/*
 * ============================================================
 * INITIALIZATION
 * ============================================================
 */

void hid_keyboard_init(
    esp_hidd_dev_t *hid_dev
)
{
    s_hid_dev = hid_dev;

    ESP_LOGI(
        TAG,
        "HID keyboard initialized"
    );
}


/*
 * ============================================================
 * SEND RAW KEY
 * ============================================================
 *
 * Sends a single HID keyboard key code.
 *
 * Example:
 *
 *     send_keyboard_key(0x2A);
 *
 * sends Backspace.
 */

void send_keyboard_key(
    uint8_t key_code
)
{
    uint8_t buffer[8] = {0};


    if (s_hid_dev == NULL) {

        ESP_LOGW(
            TAG,
            "HID device not initialized"
        );

        return;
    }


    /*
     * Keyboard report:
     *
     * byte 0 = modifier
     * byte 1 = reserved
     * byte 2 = key 1
     * byte 3 = key 2
     * byte 4 = key 3
     * byte 5 = key 4
     * byte 6 = key 5
     * byte 7 = key 6
     */

    buffer[0] = 0;
    buffer[2] = key_code;


    ESP_LOGI(
        TAG,
        "Sending HID key code: 0x%02X",
        key_code
    );


    /*
     * --------------------------------------------------------
     * Key press
     * --------------------------------------------------------
     */

    esp_hidd_dev_input_set(
        s_hid_dev,
        0,
        1,
        buffer,
        sizeof(buffer)
    );


    /*
     * Short delay between press and release.
     */

    vTaskDelay(
        pdMS_TO_TICKS(50)
    );


    /*
     * --------------------------------------------------------
     * Key release
     * --------------------------------------------------------
     */

    memset(
        buffer,
        0,
        sizeof(buffer)
    );


    esp_hidd_dev_input_set(
        s_hid_dev,
        0,
        1,
        buffer,
        sizeof(buffer)
    );
}


/*
 * ============================================================
 * SEND KEYBOARD CHARACTER
 * ============================================================
 */

void send_keyboard(
    char c
)
{
    uint8_t buffer[8] = {0};


    if (s_hid_dev == NULL) {

        ESP_LOGW(
            TAG,
            "HID device not initialized"
        );

        return;
    }


    char_to_code(
        buffer,
        c
    );


    ESP_LOGI(
        TAG,
        "Sending HID character: '%c'",
        c
    );


    /*
     * --------------------------------------------------------
     * Key press
     * --------------------------------------------------------
     */

    esp_hidd_dev_input_set(
        s_hid_dev,
        0,
        1,
        buffer,
        sizeof(buffer)
    );


    /*
     * Short delay between press and release.
     */

    vTaskDelay(
        pdMS_TO_TICKS(50)
    );


    /*
     * --------------------------------------------------------
     * Key release
     * --------------------------------------------------------
     */

    memset(
        buffer,
        0,
        sizeof(buffer)
    );


    esp_hidd_dev_input_set(
        s_hid_dev,
        0,
        1,
        buffer,
        sizeof(buffer)
    );
}