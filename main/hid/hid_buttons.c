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
        /*
         * ----------------------------------------------------
         * LEFT MOUSE CLICK 1 SECOND + ENTER
         * ----------------------------------------------------
         */

        case HID_ACTION_MOUSE_LEFT_CLICK_ENTER:

            ESP_LOGI(
                TAG,
                "BUTTON ACTION: LEFT CLICK 1s + ENTER"
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
             * for 1 second.
             */
            vTaskDelay(
                pdMS_TO_TICKS(1000)
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

            /*
             * Send ENTER.
             *
             * HID keyboard usage ID for ENTER = 0x28.
             */
            send_keyboard_key(
                0x28
            );

            break;


        /*
         * ----------------------------------------------------
         * SEND 123654
         * ----------------------------------------------------
         */

        case HID_ACTION_SEND_123654:

            ESP_LOGI(
                TAG,
                "BUTTON ACTION: SEND 123654"
            );

            send_keyboard('1');

            vTaskDelay(
                pdMS_TO_TICKS(200)
            );

            send_keyboard('2');

            vTaskDelay(
                pdMS_TO_TICKS(200)
            );

            send_keyboard('3');

            vTaskDelay(
                pdMS_TO_TICKS(200)
            );

            send_keyboard('6');

            vTaskDelay(
                pdMS_TO_TICKS(200)
            );

            send_keyboard('5');

            vTaskDelay(
                pdMS_TO_TICKS(200)
            );

            send_keyboard('4');

            break;


        /*
         * ----------------------------------------------------
         * DELETE 6 CHARACTERS
         * ----------------------------------------------------
         */

        case HID_ACTION_DELETE_6_CHARS:

            ESP_LOGI(
                TAG,
                "BUTTON ACTION: DELETE 6 CHARACTERS"
            );

            /*
             * HID usage ID for BACKSPACE = 0x2A.
             */

            for (int i = 0; i < 6; i++)
            {
                send_keyboard_key(
                    0x2A
                );

                /*
                 * Small interval between Backspaces.
                 */
                vTaskDelay(
                    pdMS_TO_TICKS(50)
                );
            }

            break;


        /*
         * ----------------------------------------------------
         * NONE
         * ----------------------------------------------------
         */

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
        "GPIO4 = LEFT CLICK 1s + ENTER"
    );

    ESP_LOGI(
        TAG,
        "GPIO5 = 123654"
    );

    ESP_LOGI(
        TAG,
        "GPIO6 = DELETE 6 CHARACTERS"
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