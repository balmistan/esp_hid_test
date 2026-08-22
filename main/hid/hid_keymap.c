#include "hid_keymap.h"


hid_action_t hid_keymap_get_action(uint8_t gpio)
{
    switch (gpio)
    {
        case HID_BUTTON_1_GPIO:
            return HID_ACTION_KEY_A;

        case HID_BUTTON_2_GPIO:
            return HID_ACTION_KEY_BACKSPACE;

        case HID_BUTTON_3_GPIO:
            return HID_ACTION_MOUSE_LEFT_CLICK;

        default:
            return HID_ACTION_NONE;
    }
}