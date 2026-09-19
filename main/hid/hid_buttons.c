#include "hid_buttons.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"

#include "hid_keymap.h"
#include "hid_keyboard.h"
#include "hid_mouse.h"

#include <string.h>

static const char *TAG = "HID_BUTTONS";

/*
 * HID device used by this module.
 */
static esp_hidd_dev_t *s_hid_dev = NULL;

/*
 * Task handle used by the GPIO ISR to wake the button task.
 */
static TaskHandle_t s_button_task_handle = NULL;

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
        "HID buttons initialized");
}

/*
 * ============================================================
 * GPIO INTERRUPT
 * ============================================================
 */

/*
 * GPIO interrupt handler.
 *
 * IMPORTANT:
 * Do not execute HID actions here.
 * The ISR only wakes the button task.
 */
static void IRAM_ATTR button_gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;

    BaseType_t higher_priority_task_woken = pdFALSE;

    xTaskNotifyFromISR(
        s_button_task_handle,
        gpio_num,
        eSetBits,
        &higher_priority_task_woken);

    if (higher_priority_task_woken)
    {
        portYIELD_FROM_ISR();
    }
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

        /*
         * Buttons are active LOW:
         *
         * HIGH = released
         * LOW  = pressed
         *
         * Therefore we react to the falling edge.
         */
        .intr_type = GPIO_INTR_NEGEDGE};

    ESP_ERROR_CHECK(
        gpio_config(&io_conf));
}

/*
 * ============================================================
 * ACTION EXECUTION
 * ============================================================
 */

static void execute_action(hid_action_t action)
{
    if (s_hid_dev == NULL)
    {
        ESP_LOGW(
            TAG,
            "HID device not initialized");

        return;
    }

    switch (action)
    {
        /*
         * ----------------------------------------------------
         * GPIO4
         *
         * 1. ENTER
         * 2. MOVE CURSOR 100 PIXELS UP
         * 3. LEFT CLICK 1 SECOND
         * 4. SEND 123654
         * 5. WAIT 5 SECONDS
         * 6. DELETE 6 CHARACTERS
         * ----------------------------------------------------
         */

    case HID_ACTION_MOUSE_LEFT_CLICK_ENTER:

        ESP_LOGI(
            TAG,
            "BUTTON ACTION: GPIO4 MACRO");

        /*
         * ==================================================
         * MOVE CURSOR 100 PIXELS UP
         * ==================================================
         */

        hid_mouse_send(
            s_hid_dev,
            0,
            0,
            -100,
            0);

        vTaskDelay(
            pdMS_TO_TICKS(100));

        /*
         * ==================================================
         * LEFT CLICK 1 SECOND
         * ==================================================
         */

        hid_mouse_send(
            s_hid_dev,
            1,
            0,
            0,
            0);

        vTaskDelay(
            pdMS_TO_TICKS(1000));

        /*
         * Release left mouse button.
         */

        hid_mouse_send(
            s_hid_dev,
            0,
            0,
            0,
            0);

        /*
         * ==================================================
         * SEND ENTER
         * ==================================================
         */

        vTaskDelay(
            pdMS_TO_TICKS(100));

        send_keyboard_key(
            0x28);

        /*
         * Pause before sending characters.
         */

        vTaskDelay(
            pdMS_TO_TICKS(1000));

        /*
         * ==================================================
         * SEND 123654
         * ==================================================
         */

        send_keyboard('1');

        vTaskDelay(
            pdMS_TO_TICKS(50));

        send_keyboard('2');

        vTaskDelay(
            pdMS_TO_TICKS(50));

        send_keyboard('3');

        vTaskDelay(
            pdMS_TO_TICKS(50));

        send_keyboard('6');

        vTaskDelay(
            pdMS_TO_TICKS(50));

        send_keyboard('5');

        vTaskDelay(
            pdMS_TO_TICKS(50));

        send_keyboard('4');

        /*
         * ==================================================
         * WAIT 5 SECONDS
         * ==================================================
         */

        vTaskDelay(
            pdMS_TO_TICKS(5000));

        /*
         * ==================================================
         * DELETE 6 CHARACTERS
         * ==================================================
         */

        for (int i = 0; i < 6; i++)
        {
            send_keyboard_key(
                0x2A);

            vTaskDelay(
                pdMS_TO_TICKS(50));
        }

        break;

        /*
         * ----------------------------------------------------
         * GPIO5
         *
         * SEND 123654
         * ----------------------------------------------------
         */

    case HID_ACTION_SEND_123654:

        ESP_LOGI(
            TAG,
            "BUTTON ACTION: SEND 123654");

        send_keyboard('1');

        vTaskDelay(
            pdMS_TO_TICKS(50));

        send_keyboard('2');

        vTaskDelay(
            pdMS_TO_TICKS(50));

        send_keyboard('3');

        vTaskDelay(
            pdMS_TO_TICKS(50));

        send_keyboard('6');

        vTaskDelay(
            pdMS_TO_TICKS(50));

        send_keyboard('5');

        vTaskDelay(
            pdMS_TO_TICKS(50));

        send_keyboard('4');

        break;

        /*
         * ----------------------------------------------------
         * GPIO6
         *
         * DELETE 2 CHARACTERS AND SEND THE PIN
         * ----------------------------------------------------
         */

    case HID_ACTION_DELETE_CHARS_AND_PINSEND:
    {
        ESP_LOGI(
            TAG,
            "BUTTON ACTION: DELETE 2 CHARACTERS AND SEND THE PIN");

        int delay = 20;
        char pin[] = "12365400";

        send_keyboard_key(
            0x2A);

        vTaskDelay(
            pdMS_TO_TICKS(1000));

        send_keyboard_key(
            0x2A);

        vTaskDelay(
            pdMS_TO_TICKS(1000));

        /*
         * SEND THE PIN
         */

        for (int i = 0; i < strlen(pin); i++)
        {
            vTaskDelay(
                pdMS_TO_TICKS(delay));

            send_keyboard(pin[i]);
        }

        break;
    }

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
        "HID BUTTON TASK STARTED");

    /*
     * Save task handle so the ISR can wake this task.
     */
    s_button_task_handle = xTaskGetCurrentTaskHandle();

    /*
     * Configure GPIOs.
     */

    configure_button_gpio(
        HID_BUTTON_1_GPIO);

    configure_button_gpio(
        HID_BUTTON_2_GPIO);

    configure_button_gpio(
        HID_BUTTON_3_GPIO);

    /*
     * Install GPIO ISR service.
     *
     * No special ISR flags are required here.
     */
    esp_err_t ret =
        gpio_install_isr_service(0);

    if (ret != ESP_OK &&
        ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }

    /*
     * Register individual handlers.
     */

    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            HID_BUTTON_1_GPIO,
            button_gpio_isr_handler,
            (void *)HID_BUTTON_1_GPIO));

    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            HID_BUTTON_2_GPIO,
            button_gpio_isr_handler,
            (void *)HID_BUTTON_2_GPIO));

    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            HID_BUTTON_3_GPIO,
            button_gpio_isr_handler,
            (void *)HID_BUTTON_3_GPIO));

    ESP_LOGI(
        TAG,
        "GPIO4 = ENTER + MOVE UP 100px + CLICK 1s + 123654 + DELETE");

    ESP_LOGI(
        TAG,
        "GPIO5 = 123654");

    ESP_LOGI(
        TAG,
        "GPIO6 = DELETE 2 CHARACTERS + PIN");

    ESP_LOGI(
        TAG,
        "GPIO interrupt mode active");

    /*
     * ========================================================
     * WAIT FOR BUTTON INTERRUPTS
     * ========================================================
     */

    while (1)
    {
        uint32_t notification = 0;

        /*
         * Sleep indefinitely until a GPIO interrupt
         * wakes this task.
         *
         * This replaces the 100 ms polling loop.
         */
        xTaskNotifyWait(
            0,
            UINT32_MAX,
            &notification,
            portMAX_DELAY);

        /*
         * ----------------------------------------------------
         * GPIO4
         * ----------------------------------------------------
         */

        if (notification &
            (1UL << HID_BUTTON_1_GPIO))
        {
            /*
             * Confirm that the button is actually LOW.
             *
             * This also helps reject some bounce events.
             */
            if (gpio_get_level(
                    HID_BUTTON_1_GPIO) == 0)
            {
                execute_action(
                    hid_keymap_get_action(
                        HID_BUTTON_1_GPIO));

                /*
                 * Simple debounce.
                 */
                vTaskDelay(
                    pdMS_TO_TICKS(50));
            }
        }

        /*
         * ----------------------------------------------------
         * GPIO5
         * ----------------------------------------------------
         */

        if (notification &
            (1UL << HID_BUTTON_2_GPIO))
        {
            if (gpio_get_level(
                    HID_BUTTON_2_GPIO) == 0)
            {
                execute_action(
                    hid_keymap_get_action(
                        HID_BUTTON_2_GPIO));

                vTaskDelay(
                    pdMS_TO_TICKS(50));
            }
        }

        /*
         * ----------------------------------------------------
         * GPIO6
         * ----------------------------------------------------
         */

        if (notification &
            (1UL << HID_BUTTON_3_GPIO))
        {
            if (gpio_get_level(
                    HID_BUTTON_3_GPIO) == 0)
            {
                execute_action(
                    hid_keymap_get_action(
                        HID_BUTTON_3_GPIO));

                vTaskDelay(
                    pdMS_TO_TICKS(50));
            }
        }
    }
}