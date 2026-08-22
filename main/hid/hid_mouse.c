#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_hidd.h"

#include "hid/hid_mouse.h"


/*
 * ============================================================
 * MOUSE REPORT MAP
 * ============================================================
 *
 * This report is used as:
 *
 * Report ID 2 = Mouse
 *
 * The combined keyboard + mouse report map is defined
 * separately in hid_report_map.h.
 */

const unsigned char mouseReportMap[] = {

    0x05, 0x01,
    0x09, 0x02,
    0xA1, 0x01,

    0x09, 0x01,
    0xA1, 0x00,

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


/*
 * ============================================================
 * SEND MOUSE REPORT
 * ============================================================
 *
 * Combined HID device:
 *
 * Report ID 1 = Keyboard
 * Report ID 2 = Mouse
 *
 * Mouse report:
 *
 * byte 0 = buttons
 * byte 1 = X
 * byte 2 = Y
 * byte 3 = wheel
 */

void hid_mouse_send(
    esp_hidd_dev_t *hid_dev,
    uint8_t buttons,
    int8_t dx,
    int8_t dy,
    int8_t wheel)
{
    uint8_t buffer[4] = {
        buttons,
        (uint8_t)dx,
        (uint8_t)dy,
        (uint8_t)wheel
    };


    /*
     * Send mouse report.
     *
     * map_index = 0
     * report_id = 2
     */
    esp_hidd_dev_input_set(
        hid_dev,
        0,
        2,
        buffer,
        sizeof(buffer)
    );
}


/*
 * ============================================================
 * MOUSE DEMO TASK
 * ============================================================
 *
 * This task is used when the device is configured as
 * mouse-only (ROLE == 3).
 *
 * In the combined keyboard + mouse configuration,
 * physical buttons are handled by hid_buttons.c.
 */

void hid_mouse_task(void *pvParameters)
{
    esp_hidd_dev_t *hid_dev =
        (esp_hidd_dev_t *)pvParameters;


    static const char *help_string =
        "########################################################################\n"
        "BT hid mouse demo usage:\n"
        "You can input these value to simulate mouse: "
        "'q', 'w', 'e', 'a', 's', 'd', 'h'\n"
        "q -- click the left key\n"
        "w -- move up\n"
        "e -- click the right key\n"
        "a -- move left\n"
        "s -- move down\n"
        "d -- move right\n"
        "h -- show the help\n"
        "########################################################################\n";


    printf(
        "%s\n",
        help_string
    );


    char c;


    while (1) {

        c = fgetc(stdin);


        switch (c) {

        /*
         * ----------------------------------------------------
         * LEFT CLICK
         * ----------------------------------------------------
         */

        case 'q':

            /*
             * Button 1 pressed.
             */
            hid_mouse_send(
                hid_dev,
                1,
                0,
                0,
                0
            );


            /*
             * Button 1 released.
             */
            vTaskDelay(
                pdMS_TO_TICKS(30)
            );


            hid_mouse_send(
                hid_dev,
                0,
                0,
                0,
                0
            );

            break;


        /*
         * ----------------------------------------------------
         * MOVE UP
         * ----------------------------------------------------
         */

        case 'w':

            hid_mouse_send(
                hid_dev,
                0,
                0,
                -10,
                0
            );

            break;


        /*
         * ----------------------------------------------------
         * RIGHT CLICK
         * ----------------------------------------------------
         */

        case 'e':

            /*
             * Button 2 pressed.
             */
            hid_mouse_send(
                hid_dev,
                2,
                0,
                0,
                0
            );


            /*
             * Button 2 released.
             */
            vTaskDelay(
                pdMS_TO_TICKS(30)
            );


            hid_mouse_send(
                hid_dev,
                0,
                0,
                0,
                0
            );

            break;


        /*
         * ----------------------------------------------------
         * MOVE LEFT
         * ----------------------------------------------------
         */

        case 'a':

            hid_mouse_send(
                hid_dev,
                0,
                -10,
                0,
                0
            );

            break;


        /*
         * ----------------------------------------------------
         * MOVE DOWN
         * ----------------------------------------------------
         */

        case 's':

            hid_mouse_send(
                hid_dev,
                0,
                0,
                10,
                0
            );

            break;


        /*
         * ----------------------------------------------------
         * MOVE RIGHT
         * ----------------------------------------------------
         */

        case 'd':

            hid_mouse_send(
                hid_dev,
                0,
                10,
                0,
                0
            );

            break;


        /*
         * ----------------------------------------------------
         * HELP
         * ----------------------------------------------------
         */

        case 'h':

            printf(
                "%s\n",
                help_string
            );

            break;


        default:

            break;
        }


        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}