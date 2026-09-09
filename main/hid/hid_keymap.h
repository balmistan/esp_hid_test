#ifndef HID_KEYMAP_H
#define HID_KEYMAP_H

#include <stdint.h>

typedef enum
{
    HID_ACTION_NONE = 0,

    HID_ACTION_MOUSE_LEFT_CLICK_ENTER,
    HID_ACTION_SEND_123654,
    HID_ACTION_DELETE_2_CHARS_SEND_123654

} hid_action_t;


/*
 * GPIO button configuration.
 */

#define HID_BUTTON_1_GPIO 4
#define HID_BUTTON_2_GPIO 5
#define HID_BUTTON_3_GPIO 6


/*
 * Return the action assigned to a GPIO.
 */

hid_action_t hid_keymap_get_action(uint8_t gpio);

#endif /* HID_KEYMAP_H */