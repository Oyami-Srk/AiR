#define DEBUG 1

#define LFS_YES_TRACE

#include "display.h"
#include "util.h"
#include "vars.h"
#include <Arduino.h>
#include <FS.h>
#include <LITTLEFS.h>

// module setup entry point
extern void wifi_setup();
extern void display_setup();
extern void http_setup();
extern void pms_setup();
extern void co2_setup();
extern void clock_setup();
extern void button_setup();
extern void data_setup();
extern void mqtt_setup();

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
    co2_setup();
    clock_setup();
    button_setup();
    if (fs_usable)
        data_setup();
    if (fs_usable)
        mqtt_setup();
}

void loop() { vTaskDelete(NULL); }