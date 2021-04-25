/*!
 * \file vars.cpp
 *
 * This is declaration and initialization of variables used across the whole
 * project
 *
 * \copyright GNU Public License V3.0
 */
#include "vars.h"
#include <Arduino.h>

bool           fs_usable;   // whether LittleFS usable
bool           wifi_enable; // whether WiFi is enabled
bool           mqtt_enable; // whether mqtt is enabled
u8_t           setup_mode;
AsyncWebServer http_server(80);
int            display_page;

SemaphoreHandle_t mutex_co2; // 4bytes manipulate is atomic, no need for lock.
SemaphoreHandle_t mutex_pms;

WiFiClient   wifi_client;
PubSubClient mqtt_client(wifi_client);
