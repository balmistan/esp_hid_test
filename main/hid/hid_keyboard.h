#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include <stdint.h>

#include "esp_hidd.h"

/**
 * BLE HID keyboard report map.
 */
extern const unsigned char keyboardReportMap[];

/**
 * Length of the BLE HID keyboard report map.
 */
#define KEYBOARD_REPORT_MAP_LEN 65

/**
 * Initialize the HID keyboard module.
 */
void hid_keyboard_init(esp_hidd_dev_t *hid_dev);

/**
 * Send a single character using the BLE HID keyboard.
 */
void send_keyboard(char c);

/**
 * FreeRTOS task responsible for monitoring the GPIO button.
 */
void ble_hid_demo_task_kbd(void *pvParameters);

#endif /* HID_KEYBOARD_H */