#ifndef __VARS_H__
#define __VARS_H__

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <freertos/semphr.h>

struct PMS_DATA {
    // Factory environment
    uint16_t PM_FE_UG_1_0;
    uint16_t PM_FE_UG_2_5;
    uint16_t PM_FE_UG_10_0;

    // Atmospheric environment
    uint16_t PM_AE_UG_1_0;
    uint16_t PM_AE_UG_2_5;
    uint16_t PM_AE_UG_10_0;

    //
    uint16_t ABOVE_0dot3_um;
    uint16_t ABOVE_0dot5_um;
    uint16_t ABOVE_1dot0_um;
    uint16_t ABOVE_2dot5_um;

    // T part2
    uint16_t TEMP;
    uint16_t HUMD;
};

extern bool           fs_usable;   // whether LittleFS usable
extern bool           wifi_enable; // whether WiFi is enabled
extern bool           mqtt_enable; // whether mqtt is enable
extern u8_t           setup_mode;  // whether in setup mode
extern AsyncWebServer http_server;
extern int            display_page;

extern SemaphoreHandle_t mutex_co2;
extern SemaphoreHandle_t mutex_pms;

extern WiFiClient   wifi_client;
extern PubSubClient mqtt_client;

#define SETUP_MODE_WIFI 0x01

#endif // __VARS_H__