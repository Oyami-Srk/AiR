//
// Created by Shiroko on 2021/4/17.
//
#include "util.h"
#include "vars.h"
#include <Arduino.h>
#include <LITTLEFS.h>
#include <ctime>

#define DATA_SAVE_INTERVAL 5000 // in ms

extern unsigned long   co2;       // co2
extern struct PMS_DATA pms_data;  // pms data
extern bool            pms_ready; // pms

#define MAX_BYTE (600 * 1024)

[[noreturn]] void task_data(void *param) {
    static portTickType       last_wake_time = xTaskGetTickCount();
    const static portTickType task_freq =
        //        pdMS_TO_TICKS(1000 * 60 * 30); // every 30 min in release
        pdMS_TO_TICKS(1000 * 120); // every 120 sec in debug

    while (co2 == 0 || !pms_ready)
        vTaskDelay(pdMS_TO_TICKS(50));

    //    vTaskDelay(pdMS_TO_TICKS(10000));

    for (;;) {
        File file = LITTLEFS.open("/data.sav", "rb+");
        file.seek(0, SeekSet);
        unsigned long position;
        file.read((uint8_t *)&position, 4);
        file.seek(position, SeekSet);
        unsigned long   local_co2;
        struct PMS_DATA local_pms_data {};

        if (xSemaphoreTake(mutex_pms, pdMS_TO_TICKS(500)) == pdTRUE) {
            memcpy(&local_pms_data, &pms_data, sizeof(pms_data));
            xSemaphoreGive(mutex_pms);
            local_co2 = co2;
            uint8_t buffer[32]; // 32 byte buffer

            // write buffer
            time_t *       pTime = (time_t *)buffer;
            unsigned long *pCO2  = (unsigned long *)(buffer + 4);
            time(pTime);
            *pCO2 = local_co2;
            memcpy(buffer + 4 + 4, &local_pms_data, sizeof(pms_data));

            // write file
            file.write(buffer, 32);
#if DEBUG
            Serial.printf("Wrote to file /data.sav at position %ld. With "
                          "buffer content: ",
                          position);
            for (unsigned char i : buffer)
                Serial.printf("0x%02X ", i);
            Serial.println();
#endif

            file.seek(0, SeekSet);
            position += 32;
            if (position + 32 > MAX_BYTE)
                position = 4;
            file.write((uint8_t *)&position, 4);
        }

        file.flush();
        file.close();
        vTaskDelayUntil(&last_wake_time, task_freq);
    }
}

void data_setup() {
    http_server.on("/api/get_raw_data", HTTP_GET,
                   [](AsyncWebServerRequest *request) {
                       request->send(LITTLEFS, "/data.sav");
                   });
    http_server.on(
        "/api/get_current_data", HTTP_GET, [](AsyncWebServerRequest *request) {
            unsigned long   local_co2;
            struct PMS_DATA local_pms_data {};

            memcpy(&local_pms_data, &pms_data, sizeof(pms_data));
            local_co2 = co2;
            time_t time_v;
            time(&time_v);
            String s = "[";
            char   str_buffer[128];
            sprintf(str_buffer, "%ld,%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                    time_v, local_co2, local_pms_data.PM_FE_UG_1_0,
                    local_pms_data.PM_FE_UG_2_5, local_pms_data.PM_FE_UG_10_0,
                    local_pms_data.PM_AE_UG_1_0, local_pms_data.PM_AE_UG_2_5,
                    local_pms_data.PM_AE_UG_10_0, local_pms_data.ABOVE_0dot3_um,
                    local_pms_data.ABOVE_0dot5_um,
                    local_pms_data.ABOVE_1dot0_um,
                    local_pms_data.ABOVE_2dot5_um, local_pms_data.TEMP,
                    local_pms_data.HUMD);
            s += str_buffer;
            s += "]";
            request->send(200, "application/json", s);
        });
    // if file not exists, create one for write
    if (!LITTLEFS.exists("/data.sav")) {
        File f = LITTLEFS.open("/data.sav", "w");
        f.write(4); // write at 5th bytes, leaving 4 bytes to save position
        f.flush();
        f.close();
    }
    xTaskCreate(task_data, "data_writer", 1024 * 4, NULL, 1, NULL);
}