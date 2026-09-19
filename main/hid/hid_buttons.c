#include "hid_buttons.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"

#include <string.h>

#include "hid_keymap.h"
#include "hid_keyboard.h"
#include "hid_mouse.h"

static const char *TAG = "HID_BUTTONS";

/*
 * HID device used by this module.
 */
static esp_hidd_dev_t *s_hid_dev = NULL;

/*
 * Button task handle.
 */
static TaskHandle_t s_button_task_handle = NULL;

/*
 * Deep sleep inactivity timer.
 */
static esp_timer_handle_t s_sleep_timer = NULL;

/*
 * ============================================================
 * DEEP SLEEP CONFIGURATION
 * ============================================================
 *
 * TEST:
 *   2 minutes
 *
 * Later change to:
 *
 *   60ULL * 60ULL * 1000000ULL
 *
 * for 1 hour.
 */

#define DEEP_SLEEP_TIMEOUT_US \
    (2ULL * 60ULL * 1000000ULL)


/*
 * ============================================================
 * DEEP SLEEP
 * ============================================================
 */

static void enter_deep_sleep(void)
{
    ESP_LOGI(
        TAG,
        "No button activity for 2 minutes - entering deep sleep");

    /*
     * GPIO4, GPIO5 and GPIO6 are RTC GPIOs
     * on ESP32-S3.
     *
     * Buttons are active LOW.
     *
     * Wake up when ANY of these GPIOs becomes LOW.
     */
    const uint64_t wakeup_mask =
        (1ULL << HID_BUTTON_1_GPIO) |
        (1ULL << HID_BUTTON_2_GPIO) |
        (1ULL << HID_BUTTON_3_GPIO);

    /*
     * Keep RTC peripheral powered so that the
     * GPIO pull-ups remain available during sleep.
     */
    ESP_ERROR_CHECK(
        esp_sleep_pd_config(
            ESP_PD_DOMAIN_RTC_PERIPH,
            ESP_PD_OPTION_ON));

    ESP_ERROR_CHECK(
        esp_sleep_enable_ext1_wakeup(
            wakeup_mask,
            ESP_EXT1_WAKEUP_ANY_LOW));

    ESP_LOGI(
        TAG,
        "Deep sleep wakeup mask: 0x%llX",
        wakeup_mask);

    ESP_LOGI(
        TAG,
        "Entering deep sleep...");

    esp_deep_sleep_start();
}


/*
 * ============================================================
 * SLEEP TIMER CALLBACK
 * ============================================================
 */

static void sleep_timer_callback(void *arg)
{
    if (s_button_task_handle != NULL)
    {
        /*
         * Bit 0 is reserved for the sleep timer.
         *
         * GPIO4 = bit 4
         * GPIO5 = bit 5
         * GPIO6 = bit 6
         */
        xTaskNotify(
            s_button_task_handle,
            1UL,
            eSetBits);
    }
}


/*
 * ============================================================
 * RESTART INACTIVITY TIMER
 * ============================================================
 */

static void restart_sleep_timer(void)
{
    if (s_sleep_timer == NULL)
    {
        return;
    }

    /*
     * Ignore the error if the timer is not currently running.
     */
    esp_timer_stop(
        s_sleep_timer);

    ESP_ERROR_CHECK(
        esp_timer_start_once(
            s_sleep_timer,
            DEEP_SLEEP_TIMEOUT_US));
}


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

        .intr_type = GPIO_INTR_NEGEDGE
    };

    ESP_ERROR_CHECK(
        gpio_config(&io_conf));
}


/*
 * ============================================================
 * GPIO INTERRUPT HANDLER
 * ============================================================
 */

static void IRAM_ATTR button_gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;

    BaseType_t higher_priority_task_woken = pdFALSE;

    /*
     * One notification bit per GPIO.
     */
    xTaskNotifyFromISR(
        s_button_task_handle,
        (1UL << gpio_num),
        eSetBits,
        &higher_priority_task_woken);

    if (higher_priority_task_woken)
    {
        portYIELD_FROM_ISR();
    }
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
         * 1. MOVE CURSOR 100 PIXELS UP
         * 2. LEFT CLICK 1 SECOND
         * 3. ENTER
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

        /*
         * Small pause to ensure the movement
         * is sent before the click.
         */
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

        /*
         * Keep left mouse button pressed
         * for 1 second.
         */
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

            send_keyboard(
                pin[i]);
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
        "HID BUTTON TASK STARTED");

    /*
     * Save task handle.
     */
    s_button_task_handle =
        xTaskGetCurrentTaskHandle();


    /*
     * ========================================================
     * CREATE DEEP SLEEP TIMER
     * ========================================================
     */

    const esp_timer_create_args_t timer_args = {
        .callback = sleep_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "deep_sleep_timer",
        .skip_unhandled_events = false
    };

    ESP_ERROR_CHECK(
        esp_timer_create(
            &timer_args,
            &s_sleep_timer));


    /*
     * ========================================================
     * CONFIGURE GPIOs
     * ========================================================
     */

    configure_button_gpio(
        HID_BUTTON_1_GPIO);

    configure_button_gpio(
        HID_BUTTON_2_GPIO);

    configure_button_gpio(
        HID_BUTTON_3_GPIO);


    /*
     * ========================================================
     * INSTALL GPIO ISR SERVICE
     * ========================================================
     */

    ESP_ERROR_CHECK(
        gpio_install_isr_service(0));


    /*
     * GPIO4
     */
    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            HID_BUTTON_1_GPIO,
            button_gpio_isr_handler,
            (void *)HID_BUTTON_1_GPIO));


    /*
     * GPIO5
     */
    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            HID_BUTTON_2_GPIO,
            button_gpio_isr_handler,
            (void *)HID_BUTTON_2_GPIO));


    /*
     * GPIO6
     */
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
        "GPIO6 = DELETE 2 + 12365400");


    /*
     * ========================================================
     * START 2-MINUTE INACTIVITY TIMER
     * ========================================================
     */

    restart_sleep_timer();

    ESP_LOGI(
        TAG,
        "Deep sleep inactivity timer started: 2 minutes");


    /*
     * ========================================================
     * EVENT LOOP
     * ========================================================
     *
     * The task now sleeps indefinitely.
     *
     * It wakes only when:
     *
     *   bit 0 = inactivity timer expired
     *   bit 4 = GPIO4 pressed
     *   bit 5 = GPIO5 pressed
     *   bit 6 = GPIO6 pressed
     */

    while (1)
    {
        uint32_t notification = 0;

        xTaskNotifyWait(
            0,
            UINT32_MAX,
            &notification,
            portMAX_DELAY);


        /*
         * ----------------------------------------------------
         * 2 MINUTES WITHOUT BUTTON ACTIVITY
         * ----------------------------------------------------
         */

        if (notification & 1UL)
        {
            enter_deep_sleep();
        }


        /*
         * ----------------------------------------------------
         * GPIO4
         * ----------------------------------------------------
         */

        if (notification &
            (1UL << HID_BUTTON_1_GPIO))
        {
            /*
             * Button activity -> restart inactivity timer.
             */
            restart_sleep_timer();

            /*
             * Simple debounce.
             */
            vTaskDelay(
                pdMS_TO_TICKS(50));

            if (gpio_get_level(
                    HID_BUTTON_1_GPIO) == 0)
            {
                execute_action(
                    hid_keymap_get_action(
                        HID_BUTTON_1_GPIO));
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
            /*
             * Button activity -> restart inactivity timer.
             */
            restart_sleep_timer();

            /*
             * Simple debounce.
             */
            vTaskDelay(
                pdMS_TO_TICKS(50));

            if (gpio_get_level(
                    HID_BUTTON_2_GPIO) == 0)
            {
                execute_action(
                    hid_keymap_get_action(
                        HID_BUTTON_2_GPIO));
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
            /*
             * Button activity -> restart inactivity timer.
             */
            restart_sleep_timer();

            /*
             * Simple debounce.
             */
            vTaskDelay(
                pdMS_TO_TICKS(50));

            if (gpio_get_level(
                    HID_BUTTON_3_GPIO) == 0)
            {
                execute_action(
                    hid_keymap_get_action(
                        HID_BUTTON_3_GPIO));
            }
        }
    }
}