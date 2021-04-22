#ifndef __VARS_H__
#define __VARS_H__

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <freertos/semphr.h>

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