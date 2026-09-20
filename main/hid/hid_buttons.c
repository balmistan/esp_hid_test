#include "hid_buttons.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include <string.h>

#include "hid_keymap.h"
#include "hid_keyboard.h"
#include "hid_mouse.h"

static const char *TAG = "HID_BUTTONS";

static esp_hidd_dev_t *s_hid_dev = NULL;
static TaskHandle_t s_button_task_handle = NULL;

#define DEEP_SLEEP_TIMEOUT_MS (30UL * 1000UL)

static void enter_deep_sleep(void)
{
    ESP_LOGI(TAG, "No button activity for 30 sec. - entering deep sleep");

    /*
     * TEST:
     * Wake from Deep Sleep only with GPIO4 going LOW.
     */
    uint64_t wakeup_mask =
        (1ULL << HID_BUTTON_1_GPIO);

    ESP_LOGI(
        TAG,
        "EXT1 wakeup mask: 0x%llX",
        (unsigned long long)wakeup_mask);

    ESP_ERROR_CHECK(
        esp_sleep_pd_config(
            ESP_PD_DOMAIN_RTC_PERIPH,
            ESP_PD_OPTION_ON));

    ESP_ERROR_CHECK(
        esp_sleep_enable_ext1_wakeup(
            wakeup_mask,
            ESP_EXT1_WAKEUP_ANY_LOW));

    ESP_LOGI(TAG, "GPIO4 level before sleep: %d",
             gpio_get_level(HID_BUTTON_1_GPIO));

    ESP_LOGI(TAG, "Entering deep sleep...");

    gpio_hold_en(HID_BUTTON_1_GPIO);
    gpio_deep_sleep_hold_en();

    esp_deep_sleep_start();
}

void hid_buttons_init(esp_hidd_dev_t *hid_dev)
{
    s_hid_dev = hid_dev;
    ESP_LOGI(TAG, "HID buttons initialized");
}

static void configure_button_gpio(uint8_t gpio)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

static void IRAM_ATTR button_gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    BaseType_t higher_priority_task_woken = pdFALSE;

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

static void execute_action(hid_action_t action)
{
    if (s_hid_dev == NULL)
    {
        ESP_LOGW(TAG, "HID device not initialized");
        return;
    }

    switch (action)
    {
        case HID_ACTION_MOUSE_LEFT_CLICK_ENTER:

            ESP_LOGI(TAG, "BUTTON ACTION: GPIO4 MACRO");

            hid_mouse_send(
                s_hid_dev,
                0,
                0,
                -100,
                0);

            vTaskDelay(pdMS_TO_TICKS(100));

            hid_mouse_send(
                s_hid_dev,
                1,
                0,
                0,
                0);

            vTaskDelay(pdMS_TO_TICKS(1000));

            hid_mouse_send(
                s_hid_dev,
                0,
                0,
                0,
                0);

            vTaskDelay(pdMS_TO_TICKS(100));

            send_keyboard_key(0x28);

            vTaskDelay(pdMS_TO_TICKS(1000));

            send_keyboard('1');
            vTaskDelay(pdMS_TO_TICKS(50));

            send_keyboard('2');
            vTaskDelay(pdMS_TO_TICKS(50));

            send_keyboard('3');
            vTaskDelay(pdMS_TO_TICKS(50));

            send_keyboard('6');
            vTaskDelay(pdMS_TO_TICKS(50));

            send_keyboard('5');
            vTaskDelay(pdMS_TO_TICKS(50));

            send_keyboard('4');

            vTaskDelay(pdMS_TO_TICKS(5000));

            for (int i = 0; i < 6; i++)
            {
                send_keyboard_key(0x2A);
                vTaskDelay(pdMS_TO_TICKS(50));
            }

            break;

        case HID_ACTION_SEND_123654:

            ESP_LOGI(TAG, "BUTTON ACTION: SEND 123654");

            send_keyboard('1');
            vTaskDelay(pdMS_TO_TICKS(50));

            send_keyboard('2');
            vTaskDelay(pdMS_TO_TICKS(50));

            send_keyboard('3');
            vTaskDelay(pdMS_TO_TICKS(50));

            send_keyboard('6');
            vTaskDelay(pdMS_TO_TICKS(50));

            send_keyboard('5');
            vTaskDelay(pdMS_TO_TICKS(50));

            send_keyboard('4');

            break;

        case HID_ACTION_DELETE_CHARS_AND_PINSEND:

            ESP_LOGI(
                TAG,
                "BUTTON ACTION: DELETE 2 CHARACTERS AND SEND THE PIN");

            {
                int delay = 20;
                char pin[] = "12365400";

                send_keyboard_key(0x2A);
                vTaskDelay(pdMS_TO_TICKS(1000));

                send_keyboard_key(0x2A);
                vTaskDelay(pdMS_TO_TICKS(1000));

                for (int i = 0; i < strlen(pin); i++)
                {
                    vTaskDelay(pdMS_TO_TICKS(delay));
                    send_keyboard(pin[i]);
                }
            }

            break;

        case HID_ACTION_NONE:
        default:
            break;
    }
}

void hid_buttons_task(void *pvParameters)
{
    ESP_LOGI(TAG, "HID BUTTON TASK STARTED");

    s_button_task_handle = xTaskGetCurrentTaskHandle();

    configure_button_gpio(HID_BUTTON_1_GPIO);
    configure_button_gpio(HID_BUTTON_2_GPIO);
    configure_button_gpio(HID_BUTTON_3_GPIO);

    ESP_ERROR_CHECK(gpio_install_isr_service(0));

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
        "GPIO4 = MOVE UP 100px + CLICK 1s + ENTER + 123654 + DELETE");

    ESP_LOGI(TAG, "GPIO5 = 123654");
    ESP_LOGI(TAG, "GPIO6 = DELETE 2 + 12365400");
    ESP_LOGI(TAG, "Deep sleep timeout started: 30 sec");

    while (1)
    {
        uint32_t notification = 0;

        BaseType_t result =
            xTaskNotifyWait(
                0,
                UINT32_MAX,
                &notification,
                pdMS_TO_TICKS(DEEP_SLEEP_TIMEOUT_MS));

        if (result == pdFALSE)
        {
            enter_deep_sleep();
            continue;
        }

        if (notification &
            (1UL << HID_BUTTON_1_GPIO))
        {
            vTaskDelay(pdMS_TO_TICKS(50));

            if (gpio_get_level(HID_BUTTON_1_GPIO) == 0)
            {
                execute_action(
                    hid_keymap_get_action(
                        HID_BUTTON_1_GPIO));
            }
        }

        if (notification &
            (1UL << HID_BUTTON_2_GPIO))
        {
            vTaskDelay(pdMS_TO_TICKS(50));

            if (gpio_get_level(HID_BUTTON_2_GPIO) == 0)
            {
                execute_action(
                    hid_keymap_get_action(
                        HID_BUTTON_2_GPIO));
            }
        }

        if (notification &
            (1UL << HID_BUTTON_3_GPIO))
        {
            vTaskDelay(pdMS_TO_TICKS(50));

            if (gpio_get_level(HID_BUTTON_3_GPIO) == 0)
            {
                execute_action(
                    hid_keymap_get_action(
                        HID_BUTTON_3_GPIO));
            }
        }
    }
}
