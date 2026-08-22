#include "hid_buttons.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"

#include "hid/hid_keymap.h"
#include "hid/hid_keyboard.h"
#include "hid/hid_mouse.h"


static const char *TAG = "HID_BUTTONS";


/*
 * HID device used by this module.
 */
static esp_hidd_dev_t *s_hid_dev = NULL;


/*
 * ============================================================
 * INITIALIZATION
 * ============================================================
 */

void hid_buttons_init(esp_hidd_dev_t *hid_dev)
{
    s_hid_dev = hid_dev;

    ESP_LOGI(
        TAG,
        "HID buttons initialized"
    );
}


/*
 * ============================================================
 * GPIO CONFIGURATION
 * ============================================================
 */

static void configure_button_gpio(uint8_t gpio)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio),

        .mode = GPIO_MODE_INPUT,

        .pull_up_en = GPIO_PULLUP_ENABLE,

        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(
        gpio_config(&io_conf)
    );
}


/*
 * ============================================================
 * ACTION EXECUTION
 * ============================================================
 */

static void execute_action(hid_action_t action)
{
    if (s_hid_dev == NULL) {

        ESP_LOGW(
            TAG,
            "HID device not initialized"
        );

        return;
    }


    switch (action)
    {
        case HID_ACTION_KEY_A:

            ESP_LOGI(
                TAG,
                "BUTTON ACTION: A"
            );

            send_keyboard('a');

            break;


        case HID_ACTION_KEY_BACKSPACE:

            ESP_LOGI(
                TAG,
                "BUTTON ACTION: BACKSPACE"
            );

            send_keyboard_key(
                0x2A
            );

            break;


        case HID_ACTION_MOUSE_LEFT_CLICK:

   ESP_LOGI(
        TAG,
        "BUTTON ACTION: LEFT CLICK"
    );

    /*
     * Mouse button pressed.
     */
    hid_mouse_send(
        s_hid_dev,
        1,
        0,
        0,
        0
    );

    /*
     * Keep left mouse button pressed
     * for 3 seconds.
     */
    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );

    /*
     * Mouse button released.
     */
    hid_mouse_send(
        s_hid_dev,
        0,
        0,
        0,
        0
    );

    break;


        case HID_ACTION_NONE:

        default:

            break;
    }
}


/*
 * ============================================================
 * BUTTON TASK
 * ============================================================
 */

void hid_buttons_task(void *pvParameters)
{
    ESP_LOGI(
        TAG,
        "HID BUTTON TASK STARTED"
    );


    configure_button_gpio(
        HID_BUTTON_1_GPIO
    );

    configure_button_gpio(
        HID_BUTTON_2_GPIO
    );

    configure_button_gpio(
        HID_BUTTON_3_GPIO
    );


    ESP_LOGI(
        TAG,
        "GPIO4 = A"
    );

    ESP_LOGI(
        TAG,
        "GPIO5 = BACKSPACE"
    );

    ESP_LOGI(
        TAG,
        "GPIO6 = LEFT CLICK"
    );


    /*
     * GPIOs use internal pull-ups:
     *
     * HIGH = not pressed
     * LOW  = pressed
     */

    int last_state_1 = 1;
    int last_state_2 = 1;
    int last_state_3 = 1;


    while (1)
    {
        int state_1 =
            gpio_get_level(
                HID_BUTTON_1_GPIO
            );

        int state_2 =
            gpio_get_level(
                HID_BUTTON_2_GPIO
            );

        int state_3 =
            gpio_get_level(
                HID_BUTTON_3_GPIO
            );


        /*
         * ----------------------------------------------------
         * GPIO4
         * ----------------------------------------------------
         */

        if (state_1 == 0 &&
            last_state_1 == 1)
        {
            execute_action(
                hid_keymap_get_action(
                    HID_BUTTON_1_GPIO
                )
            );

            vTaskDelay(
                pdMS_TO_TICKS(50)
            );
        }


        /*
         * ----------------------------------------------------
         * GPIO5
         * ----------------------------------------------------
         */

        if (state_2 == 0 &&
            last_state_2 == 1)
        {
            execute_action(
                hid_keymap_get_action(
                    HID_BUTTON_2_GPIO
                )
            );

            vTaskDelay(
                pdMS_TO_TICKS(50)
            );
        }


        /*
         * ----------------------------------------------------
         * GPIO6
         * ----------------------------------------------------
         */

        if (state_3 == 0 &&
            last_state_3 == 1)
        {
            execute_action(
                hid_keymap_get_action(
                    HID_BUTTON_3_GPIO
                )
            );

            vTaskDelay(
                pdMS_TO_TICKS(50)
            );
        }


        last_state_1 = state_1;
        last_state_2 = state_2;
        last_state_3 = state_3;


        /*
         * Poll every 10 ms.
         */
        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}