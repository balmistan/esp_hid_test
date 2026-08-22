#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include <stdint.h>

#include "esp_hidd.h"

extern const unsigned char keyboardReportMap[];
/*
 * BLE HID combined keyboard + mouse report map.
 *
 * Report ID 1 = Keyboard
 * Report ID 2 = Mouse
 */


/*
 * Length of the combined keyboard + mouse
 * BLE HID report map.
 */
#define KEYBOARD_MOUSE_REPORT_MAP_LEN \
    (sizeof(keyboardMouseReportMap))


/*
 * Initialize the HID keyboard module.
 */
void hid_keyboard_init(
    esp_hidd_dev_t *hid_dev
);


/*
 * Send a single ASCII character.
 */
void send_keyboard(
    char c
);


/*
 * Send a raw HID keyboard key code.
 *
 * Example:
 *
 *     send_keyboard_key(0x2A);
 *
 * sends Backspace.
 */
void send_keyboard_key(
    uint8_t key_code
);


#endif /* HID_KEYBOARD_H */