/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_sleep.h"

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/gpio.h"

#if CONFIG_BT_NIMBLE_ENABLED

#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#endif

#include "esp_hidd.h"
#include "esp_hid_gap.h"

#include "hid/hid_keyboard.h"
#include "hid/hid_mouse.h"
#include "hid/hid_consumer.h"
#include "hid/hid_buttons.h"


/*
 * ============================================================
 * GLOBAL
 * ============================================================
 */

static const char *TAG = "HID_DEV_DEMO";


/*
 * ============================================================
 * LOCAL HID PARAMETERS
 * ============================================================
 */

typedef struct
{
    TaskHandle_t task_hdl;
    esp_hidd_dev_t *hid_dev;
    uint8_t protocol_mode;
    uint8_t *buffer;
} local_param_t;


#if CONFIG_BT_BLE_ENABLED || CONFIG_BT_NIMBLE_ENABLED

static local_param_t s_ble_hid_param = {0};


/*
 * ============================================================
 * MEDIA REPORT MAP
 * ============================================================
 */

const unsigned char mediaReportMap[] = {

    0x05, 0x0C,
    0x09, 0x01,
    0xA1, 0x01,

    0x85, 0x03,

    0x09, 0x02,
    0xA1, 0x02,

    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x0A,

    0x15, 0x01,
    0x25, 0x0A,

    0x75, 0x04,
    0x95, 0x01,

    0x81, 0x00,

    0xC0,

    0x05, 0x0C,

    0x09, 0x86,

    0x15, 0xFF,
    0x25, 0x01,

    0x75, 0x02,
    0x95, 0x01,

    0x81, 0x46,

    0x09, 0xE9,
    0x09, 0xEA,

    0x15, 0x00,

    0x75, 0x01,
    0x95, 0x02,

    0x81, 0x02,

    0x09, 0xE2,
    0x09, 0x30,
    0x09, 0x83,
    0x09, 0x81,
    0x09, 0xB0,
    0x09, 0xB1,
    0x09, 0xB2,
    0x09, 0xB3,
    0x09, 0xB4,
    0x09, 0xB5,
    0x09, 0xB6,
    0x09, 0xB7,

    0x15, 0x01,
    0x25, 0x0C,

    0x75, 0x04,
    0x95, 0x01,

    0x81, 0x00,

    0x09, 0x80,

    0xA1, 0x02,

    0x05, 0x09,

    0x19, 0x01,
    0x29, 0x03,

    0x15, 0x01,
    0x25, 0x03,

    0x75, 0x02,

    0x81, 0x00,

    0xC0,

    0x81, 0x03,

    0xC0
};

#endif


/*
 * ============================================================
 * COMBINED KEYBOARD + MOUSE REPORT MAP
 * ============================================================
 *
 * Report ID 1 = Keyboard
 * Report ID 2 = Mouse
 *
 * The keyboard report map is defined in hid_keyboard.c.
 * The mouse report map is defined in hid_mouse.c.
 *
 * This combined map is used when:
 *
 * CONFIG_EXAMPLE_HID_DEVICE_ROLE == 2
 *
 * ============================================================
 */

#if CONFIG_EXAMPLE_HID_DEVICE_ROLE == 2

static const unsigned char keyboardMouseReportMap[] = {

    /*
     * ========================================================
     * KEYBOARD
     * ========================================================
     */

    0x05, 0x01,
    0x09, 0x06,
    0xA1, 0x01,

    0x85, 0x01,

    0x05, 0x07,

    0x19, 0xE0,
    0x29, 0xE7,

    0x15, 0x00,
    0x25, 0x01,

    0x75, 0x01,
    0x95, 0x08,

    0x81, 0x02,

    0x95, 0x01,
    0x75, 0x08,

    0x81, 0x03,

    0x95, 0x05,
    0x75, 0x01,

    0x05, 0x08,

    0x19, 0x01,
    0x29, 0x05,

    0x91, 0x02,

    0x95, 0x01,
    0x75, 0x03,

    0x91, 0x03,

    0x95, 0x05,
    0x75, 0x08,

    0x15, 0x00,
    0x25, 0x65,

    0x05, 0x07,

    0x19, 0x00,
    0x29, 0x65,

    0x81, 0x00,

    0xC0,


    /*
     * ========================================================
     * MOUSE
     * ========================================================
     */

    0x05, 0x01,
    0x09, 0x02,
    0xA1, 0x01,

    0x09, 0x01,
    0xA1, 0x00,

    /*
     * Mouse Report ID = 2
     */
    0x85, 0x02,

    0x05, 0x09,

    0x19, 0x01,
    0x29, 0x03,

    0x15, 0x00,
    0x25, 0x01,

    0x95, 0x03,
    0x75, 0x01,

    0x81, 0x02,

    0x95, 0x01,
    0x75, 0x05,

    0x81, 0x03,

    0x05, 0x01,

    0x09, 0x30,
    0x09, 0x31,
    0x09, 0x38,

    0x15, 0x81,
    0x25, 0x7F,

    0x75, 0x08,
    0x95, 0x03,

    0x81, 0x06,

    0xC0,
    0xC0
};

#define KEYBOARD_MOUSE_REPORT_MAP_LEN \
    (sizeof(keyboardMouseReportMap))

#endif


/*
 * ============================================================
 * BLE REPORT MAP
 * ============================================================
 */

static esp_hid_raw_report_map_t ble_report_maps[] = {

#if !CONFIG_BT_NIMBLE_ENABLED || \
    CONFIG_EXAMPLE_HID_DEVICE_ROLE == 1

    {
        .data = mediaReportMap,
        .len = sizeof(mediaReportMap)
    },

#elif CONFIG_EXAMPLE_HID_DEVICE_ROLE == 2

    {
        .data = keyboardMouseReportMap,
        .len = KEYBOARD_MOUSE_REPORT_MAP_LEN
    },

#elif CONFIG_EXAMPLE_HID_DEVICE_ROLE == 3

    {
        .data = mouseReportMap,
        .len = sizeof(mouseReportMap)
    },

#endif
};


/*
 * ============================================================
 * BLE HID CONFIGURATION
 * ============================================================
 */

static esp_hid_device_config_t ble_hid_config = {

    .vendor_id = 0x16C0,

    .product_id = 0x05DF,

    .version = 0x0100,

#if CONFIG_EXAMPLE_HID_DEVICE_ROLE == 2

    .device_name = "ESP Keyboard",

#elif CONFIG_EXAMPLE_HID_DEVICE_ROLE == 3

    .device_name = "ESP Mouse",

#else

    .device_name = "ESP BLE HID2",

#endif

    .manufacturer_name = "Espressif",

    .serial_number = "1234567890",

    .report_maps = ble_report_maps,

    .report_maps_len = 1
};


/*
 * ============================================================
 * HID TASK START / STOP
 * ============================================================
 */

void ble_hid_task_start_up(void)
{
    /*
     * Do not start the task more than once.
     */
    if (s_ble_hid_param.task_hdl) {
        return;
    }


#if CONFIG_EXAMPLE_HID_DEVICE_ROLE == 1

    /*
     * Consumer / media HID demo.
     */
    xTaskCreate(
        ble_hid_demo_task,
        "ble_hid_demo_task",
        3 * 1024,
        NULL,
        configMAX_PRIORITIES - 3,
        &s_ble_hid_param.task_hdl
    );


#elif CONFIG_EXAMPLE_HID_DEVICE_ROLE == 2

    /*
     * ========================================================
     * COMBINED KEYBOARD + MOUSE
     * ========================================================
     *
     * Report ID 1 = Keyboard
     * Report ID 2 = Mouse
     *
     * Physical buttons are handled by hid_buttons.c.
     */


    /*
     * Initialize keyboard module.
     */
    hid_keyboard_init(
        s_ble_hid_param.hid_dev
    );


    /*
     * Initialize physical button module.
     */
    hid_buttons_init(
        s_ble_hid_param.hid_dev
    );


    /*
     * Start physical button task.
     *
     * GPIO4 = A
     * GPIO5 = BACKSPACE
     * GPIO6 = LEFT CLICK
     *
     * GPIO -> action association is defined
     * in hid_keymap.c.
     */
    xTaskCreate(
        hid_buttons_task,
        "hid_buttons_task",
        3 * 1024,
        NULL,
        configMAX_PRIORITIES - 3,
        &s_ble_hid_param.task_hdl
    );


#elif CONFIG_EXAMPLE_HID_DEVICE_ROLE == 3

    /*
     * Mouse-only device.
     */
    xTaskCreate(
        hid_mouse_task,
        "hid_mouse_task",
        3 * 1024,
        s_ble_hid_param.hid_dev,
        configMAX_PRIORITIES - 3,
        &s_ble_hid_param.task_hdl
    );

#endif
}


void ble_hid_task_shut_down(void)
{
    if (s_ble_hid_param.task_hdl) {

        vTaskDelete(
            s_ble_hid_param.task_hdl
        );

        s_ble_hid_param.task_hdl = NULL;
    }
}


/*
 * ============================================================
 * BLE HID EVENT CALLBACK
 * ============================================================
 */

static void ble_hidd_event_callback(
    void *handler_args,
    esp_event_base_t base,
    int32_t id,
    void *event_data)
{
    esp_hidd_event_t event =
        (esp_hidd_event_t)id;

    esp_hidd_event_data_t *param =
        (esp_hidd_event_data_t *)event_data;

    static const char *TAG =
        "HID_DEV_BLE";


    switch (event) {

    case ESP_HIDD_START_EVENT:

        ESP_LOGI(
            TAG,
            "START"
        );

        /*
         * Start BLE advertising.
         */
        esp_hid_ble_gap_adv_start();

        break;


    case ESP_HIDD_CONNECT_EVENT:

        ESP_LOGI(
            TAG,
            "CONNECT"
        );

        break;


    case ESP_HIDD_PROTOCOL_MODE_EVENT:

        ESP_LOGI(
            TAG,
            "PROTOCOL MODE[%u]: %s",
            param->protocol_mode.map_index,
            param->protocol_mode.protocol_mode
                ? "REPORT"
                : "BOOT"
        );

        break;


    case ESP_HIDD_CONTROL_EVENT:

        ESP_LOGI(
            TAG,
            "CONTROL[%u]: %sSUSPEND",
            param->control.map_index,
            param->control.control
                ? "EXIT_"
                : ""
        );


        if (param->control.control) {

            /*
             * HID device resumed.
             */
            ble_hid_task_start_up();

        } else {

            /*
             * HID device suspended.
             */
            ble_hid_task_shut_down();
        }

        break;


    case ESP_HIDD_OUTPUT_EVENT:

        ESP_LOGI(
            TAG,
            "OUTPUT[%u]: %8s ID: %2u, Len: %d, Data:",
            param->output.map_index,
            esp_hid_usage_str(
                param->output.usage
            ),
            param->output.report_id,
            param->output.length
        );


        ESP_LOG_BUFFER_HEX(
            TAG,
            param->output.data,
            param->output.length
        );

        break;


    case ESP_HIDD_FEATURE_EVENT:

        ESP_LOGI(
            TAG,
            "FEATURE[%u]: %8s ID: %2u, Len: %d, Data:",
            param->feature.map_index,
            esp_hid_usage_str(
                param->feature.usage
            ),
            param->feature.report_id,
            param->feature.length
        );


        ESP_LOG_BUFFER_HEX(
            TAG,
            param->feature.data,
            param->feature.length
        );

        break;


    case ESP_HIDD_DISCONNECT_EVENT:

        ESP_LOGI(
            TAG,
            "DISCONNECT: %s",
            esp_hid_disconnect_reason_str(
                esp_hidd_dev_transport_get(
                    param->disconnect.dev
                ),
                param->disconnect.reason
            )
        );


        /*
         * Stop the active HID task.
         */
        ble_hid_task_shut_down();


        /*
         * Restart advertising so that the
         * device can be paired again.
         */
        esp_hid_ble_gap_adv_start();

        break;


    case ESP_HIDD_STOP_EVENT:

        ESP_LOGI(
            TAG,
            "STOP"
        );

        break;


    default:

        break;
    }
}


/*
 * ============================================================
 * NIMBLE HOST TASK
 * ============================================================
 */

#if CONFIG_BT_NIMBLE_ENABLED

void ble_hid_device_host_task(void *param)
{
    ESP_LOGI(
        TAG,
        "BLE Host Task Started"
    );


    /*
     * This function returns only when
     * nimble_port_stop() is executed.
     */
    nimble_port_run();

    nimble_port_freertos_deinit();
}


void ble_store_config_init(void);

#endif


/*
 * ============================================================
 * APP MAIN
 * ============================================================
 */

void app_main(void)
{

    esp_err_t ret;


/**/
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();

    ESP_LOGI("SLEEP", "Wakeup cause: %d", wakeup_cause);

    if (wakeup_cause == ESP_SLEEP_WAKEUP_EXT1)
    {
        uint64_t wakeup_status = esp_sleep_get_ext1_wakeup_status();

        ESP_LOGI("SLEEP",
                 "EXT1 wakeup GPIO mask: 0x%llX",
                 wakeup_status);
    }
/**/

#if HID_DEV_MODE == HIDD_IDLE_MODE

    ESP_LOGE(
        TAG,
        "Please turn on BLE HID!"
    );

    return;

#endif


    /*
     * ========================================================
     * NVS
     * ========================================================
     */

    ret =
        nvs_flash_init();


    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(
            nvs_flash_erase()
        );

        ret =
            nvs_flash_init();
    }


    ESP_ERROR_CHECK(ret);


    /*
     * ========================================================
     * HID GAP
     * ========================================================
 */

    ESP_LOGI(
        TAG,
        "setting hid gap, mode:%d",
        HID_DEV_MODE
    );


    ret =
        esp_hid_gap_init(
            HID_DEV_MODE
        );


    ESP_ERROR_CHECK(ret);


#if CONFIG_BT_BLE_ENABLED || CONFIG_BT_NIMBLE_ENABLED

    /*
     * ========================================================
     * BLE GAP
     * ========================================================
     */

#if CONFIG_EXAMPLE_HID_DEVICE_ROLE == 2

    /*
     * Combined keyboard + mouse device.
     *
     * The primary BLE appearance is keyboard.
     * The HID report map also contains the mouse
     * report.
     */
    ret =
        esp_hid_ble_gap_adv_init(
            ESP_HID_APPEARANCE_KEYBOARD,
            ble_hid_config.device_name
        );

#elif CONFIG_EXAMPLE_HID_DEVICE_ROLE == 3

    /*
     * Mouse-only device.
     */
    ret =
        esp_hid_ble_gap_adv_init(
            ESP_HID_APPEARANCE_MOUSE,
            ble_hid_config.device_name
        );

#else

    /*
     * Generic HID device.
     */
    ret =
        esp_hid_ble_gap_adv_init(
            ESP_HID_APPEARANCE_GENERIC,
            ble_hid_config.device_name
        );

#endif


    ESP_ERROR_CHECK(ret);


#if CONFIG_BT_BLE_ENABLED

    /*
     * Register GATTS callback when using
     * the Bluedroid BLE stack.
     */
    if ((ret =
        esp_ble_gatts_register_callback(
            esp_hidd_gatts_event_handler
        )) != ESP_OK) {

        ESP_LOGE(
            TAG,
            "GATTS register callback failed: %d",
            ret
        );

        return;
    }

#endif


    /*
     * ========================================================
     * BLE HID DEVICE
     * ========================================================
     */

    ESP_LOGI(
        TAG,
        "setting ble device"
    );


    ESP_ERROR_CHECK(
        esp_hidd_dev_init(
            &ble_hid_config,
            ESP_HID_TRANSPORT_BLE,
            ble_hidd_event_callback,
            &s_ble_hid_param.hid_dev
        )
    );

#endif


#if CONFIG_BT_NIMBLE_ENABLED

    /*
     * ========================================================
     * NIMBLE
     * ========================================================
     */

    /*
     * Initialize persistent BLE storage.
     *
     * This allows the pairing information to be retained.
     */
    ble_store_config_init();


    ble_hs_cfg.store_status_cb =
        ble_store_util_status_rr;


    /*
     * Start NimBLE host task after the
     * HID device has been initialized.
     */
    ret =
        esp_nimble_enable(
            ble_hid_device_host_task
        );


    if (ret) {

        ESP_LOGE(
            TAG,
            "esp_nimble_enable failed: %d",
            ret
        );
    }

#endif
}
