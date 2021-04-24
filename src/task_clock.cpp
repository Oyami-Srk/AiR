//
// Created by Shiroko on 2021/4/15.
//
#include "clock.h"
#include "display.h"
#include "util.h"
#include "vars.h"
#include <Arduino.h>
#include <ErriezDS1302.h>
#include <ctime>

const char *ntp_server     = "ntp.aliyun.com"; // use aliyun ntp server
const long  gmt_offset_sec = 8 * 60 * 60;      // UTF +8
const int   dst_offset_sec = 0;                // Our china does not have DST

#define DS1302_CLK_PIN 4
#define DS1302_IO_PIN  0
#define DS1302_CE_PIN  2

ErriezDS1302 rtc(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN);

[[noreturn]] void task_clock(void *param);

void clock_setup() {
    if (!rtc.begin()) {
#if DEBUG
        Serial.println("RTC not found.");
#endif
        return;
    }
    if (!rtc.isRunning()) {
        rtc.clockEnable(true);
    }

    struct timezone tz {
        .tz_minuteswest = gmt_offset_sec / 60, .tz_dsttime = 0
    };

    struct timeval tv {
        .tv_sec = rtc.getEpoch(),
        //        .tv_sec      = 1618499990, // for Debug
            .tv_usec = 0
    };

    settimeofday(&tv, &tz);

    xTaskCreate(task_clock, "clock", 1024 * 4, NULL, 1, NULL);
}

void printLocalTime() {
    struct tm time_info;
    if (!getLocalTime(&time_info)) {
#if DEBUG
        Serial.println("Failed to obtain time at print.");
#endif
        return;
    }
#if DEBUG
    Serial.println(&time_info, "%A, %B %d %Y %H:%M:%S");
#endif
}

void syncTimeNTP() {
    struct tm time_info;
    if (!getLocalTime(&time_info)) {
#if DEBUG
        Serial.println("Failed to obtain time at NTP sync.");
#endif
        return;
    } else {
#if DEBUG
        Serial.println(&time_info, "Write: %A, %B %d %Y %H:%M:%S To RTC");
#endif
        if (!rtc.write(&time_info)) {
#if DEBUG
            Serial.println("Failed to set RTC time.");
#endif
            return;
        }
    }
#if DEBUG
    Serial.println("RTC: Succeed to set time.");
#endif
}

// call it after connected to wifi
void init_clock() {
    configTime(gmt_offset_sec, dst_offset_sec, ntp_server);
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
        feed_dog();
    print_log("NTP has been setup.");
    printLocalTime();
    syncTimeNTP(); // sync RTC with ntp
}

[[noreturn]] void task_clock(void *param) {
    static portTickType       last_wake_time = xTaskGetTickCount();
    const static portTickType task_freq =
        //        pdMS_TO_TICKS(1000 * 60 * 60 * 24); // 1 day for release
        pdMS_TO_TICKS(1000 * 5); // 5 sec for debug
    for (;;) {
        vTaskDelayUntil(&last_wake_time, task_freq);
        syncTimeNTP();
    }
}
