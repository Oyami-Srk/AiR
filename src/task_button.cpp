/*!
 * \file task_button.cpp
 *
 * This is the button management module. Watching the status of buttons on the
 * board and execute corresponding function.
 *
 * \copyright GNU Public License V3.0
 */
#include "util.h"
#include "vars.h"
#include <Arduino.h>

#define BTN_1 32 //! Button 1 - Up
#define BTN_2 33 //! Button 2 - Down
#define BTN_3 15 //! Button 3 - Confirm

/*!
 * \brief Macros to simplify each button's task function.
 */
#define TASK_BTN(n, func)                                                      \
    [[noreturn]] void task_button_##n(void *param) {                           \
        for (;;) {                                                             \
            if (digitalRead(BTN_##n) == LOW) {                                 \
                vTaskDelay(pdMS_TO_TICKS(20));                                 \
                if (digitalRead(BTN_##n) == LOW) {                             \
                    func();                                                    \
                }                                                              \
                while (digitalRead(BTN_##n) == LOW)                            \
                    feed_dog();                                                \
                vTaskDelay(pdMS_TO_TICKS(100));                                \
            }                                                                  \
            feed_dog();                                                        \
            taskYIELD();                                                       \
        }                                                                      \
    }

void on_btn_1_click() {
    display_page++;
    if (display_page >= 4)
        display_page = 0;
}

// down
void on_btn_2_click() {
    if (display_page == 0)
        display_page = 3;
    else
        display_page--;
}

// confirm
void on_btn_3_click() {
    if (display_page == 3) {
        esp_restart();
    }
}

TASK_BTN(1, on_btn_1_click)
TASK_BTN(2, on_btn_2_click)
TASK_BTN(3, on_btn_3_click)

/*!
 * \brief Button setup function.
 *
 * Set the button's GPIO mode and create tasks for each button.
 */
void button_setup() {
    pinMode(BTN_1, INPUT_PULLUP);
    pinMode(BTN_2, INPUT_PULLUP);
    pinMode(BTN_3, INPUT_PULLUP);
    xTaskCreate(task_button_1, "button_1", 1024 * 2, NULL, 1, NULL);
    xTaskCreate(task_button_2, "button_2", 1024 * 2, NULL, 1, NULL);
    xTaskCreate(task_button_3, "button_3", 1024 * 2, NULL, 1, NULL);
}
