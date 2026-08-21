#include "hid_keyboard.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"


static const char *TAG = "HID_KEYBOARD";


/*
 * HID device used by this module.
 */
static esp_hidd_dev_t *s_hid_dev = NULL;

/*
 * ============================================================
 * BLE HID KEYBOARD REPORT MAP
 * ============================================================
 */

const unsigned char keyboardReportMap[] = {

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

    0xC0
};
/*
 * GPIO button.
 *
 * GPIO4 ---- button ---- GND
 *
 * HIGH = not pressed
 * LOW  = pressed
 */
#define BUTTON_GPIO GPIO_NUM_4


/*
 * USB HID modifier codes.
 */
#define USB_HID_MODIFIER_LEFT_CTRL      0x01
#define USB_HID_MODIFIER_LEFT_SHIFT     0x02
#define USB_HID_MODIFIER_LEFT_ALT       0x04

#define USB_HID_MODIFIER_RIGHT_CTRL     0x10
#define USB_HID_MODIFIER_RIGHT_SHIFT    0x20
#define USB_HID_MODIFIER_RIGHT_ALT      0x40


/*
 * USB HID key codes.
 */
#define USB_HID_SPACE                   0x2C
#define USB_HID_DOT                     0x37
#define USB_HID_NEWLINE                 0x28
#define USB_HID_FSLASH                 0x38
#define USB_HID_BSLASH                 0x31
#define USB_HID_COMMA                   0x36


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

static void char_to_code(uint8_t *buffer, char ch)
{
    if (ch >= 'a' && ch <= 'z') {

        buffer[0] = 0;

        buffer[2] =
            (uint8_t)(4 + (ch - 'a'));
    }

    else if (ch >= 'A' && ch <= 'Z') {

        buffer[0] =
            USB_HID_MODIFIER_LEFT_SHIFT;

        ch = ch - ('A' - 'a');

        buffer[2] =
            (uint8_t)(4 + (ch - 'a'));
    }

    else if (ch >= '0' && ch <= '9') {

        buffer[0] = 0;

        if (ch == '0') {

            buffer[2] = 39;

        } else {

            buffer[2] =
                (uint8_t)(30 + (ch - '1'));
        }
    }

    else {

        switch (ch) {

        CASE(' ', 0, USB_HID_SPACE);

        CASE('.', 0, USB_HID_DOT);

        CASE('\n', 0, USB_HID_NEWLINE);

        CASE('?', USB_HID_MODIFIER_LEFT_SHIFT,
             USB_HID_FSLASH);

        CASE('/', 0, USB_HID_FSLASH);

        CASE('\\', 0, USB_HID_BSLASH);

        CASE('|', USB_HID_MODIFIER_LEFT_SHIFT,
             USB_HID_BSLASH);

        CASE(',', 0, USB_HID_COMMA);

        CASE('<', USB_HID_MODIFIER_LEFT_SHIFT,
             USB_HID_COMMA);

        CASE('>', USB_HID_MODIFIER_LEFT_SHIFT,
             USB_HID_COMMA);

        CASE('@', USB_HID_MODIFIER_LEFT_SHIFT, 31);

        CASE('!', USB_HID_MODIFIER_LEFT_SHIFT, 30);

        CASE('#', USB_HID_MODIFIER_LEFT_SHIFT, 32);

        CASE('$', USB_HID_MODIFIER_LEFT_SHIFT, 33);

        CASE('%', USB_HID_MODIFIER_LEFT_SHIFT, 34);

        CASE('^', USB_HID_MODIFIER_LEFT_SHIFT, 35);

        CASE('&', USB_HID_MODIFIER_LEFT_SHIFT, 36);

        CASE('*', USB_HID_MODIFIER_LEFT_SHIFT, 37);

        CASE('(', USB_HID_MODIFIER_LEFT_SHIFT, 38);

        CASE(')', USB_HID_MODIFIER_LEFT_SHIFT, 39);

        CASE('-', 0, 0x2D);

        CASE('_', USB_HID_MODIFIER_LEFT_SHIFT, 0x2D);

        CASE('=', 0, 0x2E);

        CASE('+', USB_HID_MODIFIER_LEFT_SHIFT, 39);

        CASE(8, 0, 0x2A);

        CASE('\t', 0, 0x2B);

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

void hid_keyboard_init(esp_hidd_dev_t *hid_dev)
{
    s_hid_dev = hid_dev;

    ESP_LOGI(
        TAG,
        "HID keyboard initialized"
    );
}


/*
 * ============================================================
 * SEND KEYBOARD CHARACTER
 * ============================================================
 */

void send_keyboard(char c)
{
    static uint8_t buffer[8] = {0};

    if (s_hid_dev == NULL) {

        ESP_LOGW(
            TAG,
            "HID device not initialized"
        );

        return;
    }

    char_to_code(buffer, c);

    ESP_LOGI(
        TAG,
        "Sending HID character: '%c'",
        c
    );

    esp_hidd_dev_input_set(
        s_hid_dev,
        0,
        1,
        buffer,
        8
    );


    /*
     * Key release.
     */

    vTaskDelay(
        pdMS_TO_TICKS(50)
    );


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
        8
    );
}


/*
 * ============================================================
 * GPIO BUTTON TASK
 * ============================================================
 */

void ble_hid_demo_task_kbd(void *pvParameters)
{
    ESP_LOGI(
        TAG,
        "KEYBOARD TASK STARTED"
    );


    gpio_config_t io_conf = {
        .pin_bit_mask =
            (1ULL << BUTTON_GPIO),

        .mode =
            GPIO_MODE_INPUT,

        .pull_up_en =
            GPIO_PULLUP_ENABLE,

        .pull_down_en =
            GPIO_PULLDOWN_DISABLE,

        .intr_type =
            GPIO_INTR_DISABLE
    };


    ESP_ERROR_CHECK(
        gpio_config(&io_conf)
    );


    ESP_LOGI(
        TAG,
        "GPIO4 configured as INPUT with pull-up"
    );


    /*
     * Initial state:
     *
     * 1 = not pressed
     * 0 = pressed
     */

    int last_state = 1;


    while (1) {

        int state =
            gpio_get_level(BUTTON_GPIO);


        /*
         * Detect falling edge:
         *
         * HIGH -> LOW
         */

        if (state == 0 &&
            last_state == 1) {

            ESP_LOGI(
                TAG,
                "BUTTON PRESSED -> sending 'a'"
            );

            send_keyboard('a');


            /*
             * Debounce.
             */

            vTaskDelay(
                pdMS_TO_TICKS(50)
            );
        }


        last_state = state;


        /*
         * Poll every 10 ms.
         */

        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}