#ifndef HID_BUTTONS_H
#define HID_BUTTONS_H

#include "esp_hidd.h"


/**
 * Initialize the physical HID buttons.
 */
void hid_buttons_init(esp_hidd_dev_t *hid_dev);


/**
 * FreeRTOS task responsible for monitoring
 * the physical HID buttons.
 */
void hid_buttons_task(void *pvParameters);

#endif /* HID_BUTTONS_H */