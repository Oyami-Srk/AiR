//
// Created by Shiroko on 2021/4/7.
//

#include "vars.h"
#include <Arduino.h>
#include <LITTLEFS.h>

void http_setup() {
    http_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LITTLEFS, "/index.html");
    });
    http_server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LITTLEFS, "/index.html");
    });
    http_server.begin();
#if DEBUG
    Serial.println("Http server has been started.");
#endif
}

[[noreturn]] void task_http(void *param) {
    for (;;) {
    }
}