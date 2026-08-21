set(srcs
    "app_main.c"
    "app_main.c"
    "esp_hid_gap.c"

    "bluetooth/hid_bluetooth.c"
    "bluetooth/hid_ble.c"

    "hid/hid_common.c"
    "hid/hid_keyboard.c"
    "hid/hid_mouse.c"
    "hid/hid_reports.c"

    "input/button.c"
)

set(include_dirs
    "."
    "bluetooth"
    "hid"
    "input"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${include_dirs}

    REQUIRES
        esp_hid
        esp_driver_gpio

    PRIV_REQUIRES
        nvs_flash
)
