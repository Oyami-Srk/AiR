//
// Created by Shiroko on 2021/4/14.
//
#include "util.h"
#include "vars.h"
#include <Arduino.h>
#include <HardwareSerial.h>
//#include <SoftwareSerial.h>

#define CO2_SERIAL_BUS 2

HardwareSerial CO2(CO2_SERIAL_BUS);

byte          command_read_co2[] = {0xFF, 0x01, 0x86, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x79};
byte          response[]         = {0, 0, 0, 0, 0, 0, 0, 0, 0};
int           co2_value_scale    = 1;
unsigned long co2                = 0;

bool co2_send_command(Stream *serial = &CO2, byte *cmd = command_read_co2,
                      int cmd_len = 9, byte *resp = response,
                      int resp_len = 9) {
    while (!serial->available()) {
        serial->write(cmd, cmd_len);
        vTaskDelay(50 / portTICK_PERIOD_MS); // delay 50ms
    }

    int timeout = 0;
    while (serial->available() < resp_len) {
        timeout++;
        if (timeout > 10) {
            while (serial->available())
                serial->read();
            return false;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS); // delay 50ms
    }
    for (int i = 0; i < resp_len; i++)
        resp[i] = serial->read();
    return true;
}

unsigned long co2_read_response(byte *resp  = response,
                                int   scale = co2_value_scale) {
    if (resp[0] == 0xFF && resp[1] == 0x86)
        return scale * (resp[2] * 256 + resp[3]);
    else
        return 0;
}

[[noreturn]] void task_co2(void *param);

void co2_setup() {
    CO2.begin(9600);
    CO2.flush();
    xTaskCreate(task_co2, "CO2", 1024 * 8, NULL, 3, NULL);
}

[[noreturn]] void task_co2(void *param) {
    for (;;) {
        if (co2_send_command()) {
            if (xSemaphoreTake(mutex_co2, portMAX_DELAY) == pdTRUE) {
                co2 = co2_read_response();
                xSemaphoreGive(mutex_co2);
#if DEBUG
                Serial.printf("CO2: %ld\n", co2);
            } else {
                Serial.printf("Failed to obtain mutex lock.\n");
            }
        } else {
            Serial.println("Cannot send command.");
#else
            }
#endif
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}