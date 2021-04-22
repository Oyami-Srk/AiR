#define DEBUG 1

#define LFS_YES_TRACE

#include "display.h"
#include "util.h"
#include "vars.h"
#include <Arduino.h>
#include <FS.h>
#include <LITTLEFS.h>

extern void wifi_setup();
extern void display_setup();
extern void http_setup();
extern void pms_setup();
extern void s8_setup();
extern void clock_setup();
extern void button_setup();
extern void data_setup();
extern void mqtt_setup();

[[noreturn]] extern void task_wifi(void *);

[[noreturn]] extern void task_display(void *);

[[noreturn]] extern void task_http(void *);

[[noreturn]] extern void task_pms(void *);

[[noreturn]] extern void task_s8(void *);

extern void task_clock(void *);

void setup() {
    setup_mode   = 0; // nothing in setup by default
    display_page = 0; // home screen by default

    //    display_page = 1; // detail screen debug

    mutex_co2 = xSemaphoreCreateMutex();
    mutex_pms = xSemaphoreCreateMutex();
    xSemaphoreGive(mutex_co2); // actually no need to do that
    xSemaphoreGive(mutex_pms);

    Serial.begin(115200);
    display_setup(); // Initialize display at very first beginning

    Serial.println(
        "==========THE BEGINNING OF THE PROGRAM STARTS HERE==========");
    if (!LITTLEFS.begin()) {
        Serial.println("ERROR: LittleFS failed to initialize.");
        print_log("ERROR: LittleFS failed to init.");
        fs_usable = false;
    } else {
        fs_usable = true;

        if (false) {
            LITTLEFS.remove("/test");
            if (!LITTLEFS.exists("/test")) {
                LITTLEFS.open("/test", "w").close();
            }

            File test = LITTLEFS.open("/test", "rb+");
            for (int i = 0; i < 128; i++) {
                test.write(i);
                feed_dog();
            }
            test.flush();
            test.close();

            feed_dog();
            test = LITTLEFS.open("/test", "rb");
            for (int i = 0; i < 128; i++) {
                uint8_t buf;
                test.read(&buf, 1);
                Serial.printf("0x%02X ", buf);
            }
            test.close();

            test = LITTLEFS.open("/test", "rb+");
            Serial.println(test.peek());
            test.seek(0, SeekMode::SeekSet);
            test.write(0xFF);
            test.seek(125, SeekMode::SeekCur);
            test.write(0xAA);
            test.flush();
            test.close();

            feed_dog();
            test = LITTLEFS.open("/test", "rb");
            for (int i = 0; i < 128; i++) {
                uint8_t buf;
                test.read(&buf, 1);
                Serial.printf("0x%02X ", buf);
            }
            test.close();
        }
    }

    // print out FS info
    Serial.printf("We have %d KB left in the flash. (used %d bytes)\n",
                  LITTLEFS.totalBytes() / 1024, LITTLEFS.usedBytes());
    String log_s =
        "Remaining flash size: " +
        String((LITTLEFS.totalBytes() - LITTLEFS.usedBytes()) / 1024) + " KB";

    print_log(log_s);

    if (fs_usable)
        wifi_setup();
    http_setup();
    pms_setup();
    s8_setup();
    clock_setup();
    button_setup();
    if (fs_usable)
        data_setup();
    if (fs_usable)
        mqtt_setup();

    //    xTaskCreate(task_wifi, "wifi", 1024 * 4, NULL, 1, NULL);
    //    xTaskCreate(task_http, "task_http", 1024, NULL, 1, NULL);
    xTaskCreate(task_display, "display", 1024 * 4, NULL, 2, NULL);
    // Assign 8 kb to task
    xTaskCreate(task_pms, "PMS", 1024 * 8, NULL, 1, NULL);
    xTaskCreate(task_s8, "S8", 1024 * 8, NULL, 3, NULL);
    xTaskCreate(task_clock, "clock", 1024 * 4, NULL, 1, NULL);
}

void loop() { vTaskDelete(NULL); }