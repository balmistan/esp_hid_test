#ifndef HID_MOUSE_H
#define HID_MOUSE_H

#include <stdint.h>
#include "esp_hidd.h"

extern const unsigned char mouseReportMap[];

#define MOUSE_REPORT_MAP_LEN \
    (sizeof(mouseReportMap))

void hid_mouse_send(
    esp_hidd_dev_t *hid_dev,
    uint8_t buttons,
    int8_t dx,
    int8_t dy,
    int8_t wheel
);

void hid_mouse_task(
    void *pvParameters
);

#endif