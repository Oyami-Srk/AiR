//
// Created by Shiroko on 2021/4/13.
//

#include "PMS5003T.h"
#include "util.h"
#include "vars.h"
#include <Arduino.h>
//#define USE_SOFTWARE_SERIAL
#ifndef USE_SOFTWARE_SERIAL
#include <HardwareSerial.h>
#else
#include <SoftwareSerial.h>
#endif
#include <freertos/semphr.h>

#define PMS_GPIO 5

#ifdef USE_SOFTWARE_SERIAL
SoftwareSerial pms_serial;
#else
HardwareSerial pms_serial(0);
#endif
PMS5003T::DATA pms_data;
PMS5003T       pms(pms_serial, &pms_data);

void pms_setup() {
#ifndef USE_SOFTWARE_SERIAL
    pms_serial.begin(9600, SERIAL_8N1, PMS_GPIO, PMS_GPIO, false, 256);
#else
    pms_serial.enableIntTx(false);
    pms_serial.enableTx(false);
    pms_serial.begin(9600, SWSERIAL_8N1, PMS_GPIO, PMS_GPIO);
#endif
    pms_serial.flush();
}

[[noreturn]] void task_pms(void *param) {
    assert(sizeof(PMS5003T::DATA) == 24);
    for (;;) {
        pms_serial.begin(9600, SERIAL_8N1, PMS_GPIO, PMS_GPIO, false, 256);
        //        pms.loop();
        uint8_t buffer_start[2] = {0x42, 0x4D};
        uint8_t buffer[30];
        pms.status = PMS5003T::STATUS_WAITING;
        if (pms_serial.find(buffer_start, 2)) {
            pms_serial.readBytes(buffer, 30);
            auto     _data       = &pms_data;
            uint8_t *_payload    = buffer + 2;
            _data->PM_FE_UG_1_0  = makeWord(_payload[0], _payload[1]);
            _data->PM_FE_UG_2_5  = makeWord(_payload[2], _payload[3]);
            _data->PM_FE_UG_10_0 = makeWord(_payload[4], _payload[5]);

            // Atmospheric environment.
            _data->PM_AE_UG_1_0  = makeWord(_payload[6], _payload[7]);
            _data->PM_AE_UG_2_5  = makeWord(_payload[8], _payload[9]);
            _data->PM_AE_UG_10_0 = makeWord(_payload[10], _payload[11]);

            _data->ABOVE_0dot3_um = makeWord(_payload[12], _payload[13]);
            _data->ABOVE_0dot5_um = makeWord(_payload[14], _payload[15]);
            _data->ABOVE_1dot0_um = makeWord(_payload[16], _payload[17]);
            _data->ABOVE_2dot5_um = makeWord(_payload[18], _payload[19]);

            _data->TEMP = makeWord(_payload[20], _payload[21]);
            _data->HUMD = makeWord(_payload[22], _payload[23]);
            pms.status  = PMS5003T::STATUS_OK;
        }
        if (pms.status == PMS5003T::STATUS_OK) {
#if DEBUG
            Serial.printf("Temp: %.2f\n", pms_data.TEMP / 10.0f);
#endif
#if DEBUG
            Serial.printf("HUMD: %.2f%%\n", pms_data.HUMD / 10.0f);
#endif
        }
        pms_serial.end();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}