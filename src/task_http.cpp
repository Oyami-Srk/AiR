/*!
 * \file task_http.cpp
 *
 * This module initialize HTTP server and registered index web page.
 * Due to the using of Asynchronous Web Server, we have no task for this module.
 *
 * \copyright GNU Public License V3.0
 */
#include "vars.h"
#include <Arduino.h>
#include <LITTLEFS.h>

/*!
 * \brief HTTP Setup.
 */
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