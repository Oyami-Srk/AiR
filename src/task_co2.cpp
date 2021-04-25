/*!
 * \file task_co2.cpp
 *
 * This is module for reading CO2 sensor. Due to the atomic manipulate of 32Bits
 * value. There is no need for mutex lock. Module servers `unsigned long co2`
 * for other module to read co2 value.
 *
 * \copyright GNU Public License V3.0
 */
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

/*!
 * \brief Send command to sensor's serial.
 *
 * @param serial CO2 sensor's serial instance. Default is CO2
 * @param cmd command bytes array to be sent. Default is command_read_co2
 * @param cmd_len command length of bytes array. Default is length of
 * command_read_co2
 * @param resp bytes array to store the response from sensor. Default is
 * response
 * @param resp_len length of response bytes array. Default is 9
 * @return True for successful to send. False for failed to send.
 */
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

/*!
 * \brief Read co2 value from sensor response.
 * @param resp response bytes array. must be 4 bytes available.
 * @param scale scale value default to 1.
 * @return co2 value from response.
 */
unsigned long co2_read_response(byte *resp  = response,
                                int   scale = co2_value_scale) {
    if (resp[0] == 0xFF && resp[1] == 0x86)
        return scale * (resp[2] * 256 + resp[3]);
    else
        return 0;
}

[[noreturn]] void task_co2(void *param);

/*!
 * \brief CO2 module setup.
 * Initialize serial and flush the buffer. Create the module's task for reading
 * co2 to variable for other module.
 */
void co2_setup() {
    CO2.begin(9600);
    CO2.flush();
    xTaskCreate(task_co2, "CO2", 1024 * 8, NULL, 3, NULL);
}

/*!
 * \brief Read CO2 value every 0.5 sec.
 *
 * @param param FreeRTOS task param.
 */
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