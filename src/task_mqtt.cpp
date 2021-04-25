/*!
 * \file task_mqtt.cpp
 *
 * This module initialized MQTT service and registered MQTT setting page and
 * its HTTP-API.
 *
 * \copyright GNU Public License V3.0
 */
#include "display.h"
#include "util.h"
#include "vars.h"
#include <Arduino.h>
#include <LITTLEFS.h>

String mqtt_server, mqtt_username, mqtt_password;
uint   mqtt_port;

extern unsigned long co2;       // co2
extern PMS_DATA      pms_data;  // pms data
extern bool          pms_ready; // pms

/*!
 * \brief connect to mqtt broker by given credentials
 * @param server broker server address.
 * @param username login username, could be empty.
 * @param password login password, could be empty.
 * @param port server port, default is 1883.
 * @return
 */
bool mqtt_connect(String server, String username, String password,
                  uint port = 1883) {
    mqtt_client.setServer(server.c_str(), port);
    String  client_id = "AiR-MQTT-";
    uint8_t mac_buffer[6];
    WiFi.macAddress(mac_buffer);
    char mac_str_buffer[7];
    sprintf(mac_str_buffer, "%02X%02X%02X", mac_buffer[3], mac_buffer[4],
            mac_buffer[5]);
    client_id += mac_str_buffer;
    if (username == "")
        return mqtt_client.connect(client_id.c_str());
    if (password == "")
        return mqtt_client.connect(client_id.c_str(), username.c_str(), NULL);
    return mqtt_client.connect(client_id.c_str(), username.c_str(),
                               password.c_str());
}

/*!
 * \brief publish unsigned value to mqtt broker.
 * @param value value to be published
 * @param name value name
 */
void publish_it(unsigned long value, const char *name) {
    char addr_buf[32];
    sprintf(addr_buf, "AiR/%s", name);
    char buf[32];
    sprintf(buf, "%ld", value);
    if (mqtt_client.publish(addr_buf, buf))
        ;
#if DEBUG
    else
        Serial.println(" Failed...");
#endif
}

/*!
 * \brief publish float value to mqtt broker.
 * @param value value to be published
 * @param name value name
 */
void publish_it_float(float value, const char *name, int dig) {
    char addr_buf[32];
    sprintf(addr_buf, "AiR/%s", name);
    char buf[32];
    sprintf(buf, "%.*f", dig, value);
    if (mqtt_client.publish(addr_buf, buf))
        ;
#if DEBUG
    else
        Serial.println(" Failed...");
#endif
}

/*!
 * \brief Task for execute mqtt client's loop to keep connected with broker.
 * Process loop every 200ms.
 * @param param FreeRTOS task param
 */
[[noreturn]] void task_mqtt_loop(void *param) {
    static portTickType       last_wake_time = xTaskGetTickCount();
    const static portTickType task_freq      = pdMS_TO_TICKS(200); // 200 ms
    for (;;) {
        if (mqtt_enable) {
            mqtt_client.loop();
            if (mqtt_client.state() != MQTT_CONNECTED)
                mqtt_enable = false;
        } else {
            if (mqtt_server.length() > 0)
                mqtt_enable = mqtt_connect(mqtt_server, mqtt_username,
                                           mqtt_username, mqtt_port);
        }
        vTaskDelayUntil(&last_wake_time, task_freq);
    }
}

/*!
 * \brief data publish task. publish sensor data every 2 second.
 * @param param FreeRTOS task param.
 */
[[noreturn]] void task_mqtt_publish(void *param) {
    static portTickType       last_wake_time_2 = xTaskGetTickCount();
    const static portTickType task_freq_2      = pdMS_TO_TICKS(2000); // 2 sec
    for (;;) {
        if (mqtt_enable) {
            unsigned long   local_co2;
            struct PMS_DATA local_pms_data {};

            if (xSemaphoreTake(mutex_pms, pdMS_TO_TICKS(500)) == pdTRUE) {
                memcpy(&local_pms_data, &pms_data, sizeof(pms_data));
                xSemaphoreGive(mutex_pms);
                local_co2 = co2;
                if (local_co2) {
                    publish_it(local_co2, "CO2");
#if DEBUG
                    Serial.printf("Publishing CO2...\n");
#endif
                }
                if (local_pms_data.TEMP || local_pms_data.HUMD) {
#if DEBUG
                    Serial.printf("Publishing PMS...\n");
#endif
                    publish_it_float(local_pms_data.TEMP / 10.0f, "TEMP", 1);
                    publish_it_float(local_pms_data.HUMD / 10.0f, "HUMD", 1);
                    publish_it(local_pms_data.PM_FE_UG_1_0, "PM1.0-FE");
                    publish_it(local_pms_data.PM_FE_UG_2_5, "PM2.5-FE");
                    publish_it(local_pms_data.PM_FE_UG_10_0, "PM10-FE");
                    publish_it(local_pms_data.PM_AE_UG_1_0, "PM1.5");
                    publish_it(local_pms_data.PM_AE_UG_2_5, "PM2.5");
                    publish_it(local_pms_data.PM_AE_UG_10_0, "PM10");
                    publish_it(local_pms_data.ABOVE_0dot3_um, ">0.3um");
                    publish_it(local_pms_data.ABOVE_0dot5_um, ">0.5um");
                    publish_it(local_pms_data.ABOVE_1dot0_um, ">1.0um");
                    publish_it(local_pms_data.ABOVE_2dot5_um, ">2.5um");
                }
            }
        }
        vTaskDelayUntil(&last_wake_time_2, task_freq_2);
    }
}

/*!
 * \brief MQTT setup. Register HTTP API and setting page.
 */
void mqtt_setup() {
    http_server.on("/mqtt_setting.html", HTTP_GET,
                   [](AsyncWebServerRequest *request) {
                       request->send(LITTLEFS, "/mqtt_setting.html");
                   });
    http_server.on(
        "/api/set_mqtt", HTTP_POST, [](AsyncWebServerRequest *request) {
            if (LITTLEFS.exists("/mqtt.cfg")) {
#if DEBUG
                Serial.println("Removing existing mqtt config file.");
#endif
                LITTLEFS.remove("/mqtt.cfg");
            }
#if DEBUG
            Serial.println("Opening mqtt.cfg file to write configure.");
#endif
            File mqtt_cfg = LITTLEFS.open("/mqtt.cfg", "w");
            if (!mqtt_cfg) {
#if DEBUG
                Serial.println("Cannot write on LittleFS");
#endif
            } else {
#if DEBUG
                Serial.println("Successfully opened file.");
#endif
            }
            if (!request->hasParam("server", true)) {
                request->send(400, "text/plain", "Arguments invalid.");
                return;
            }

            String server =
                urlDecode(request->getParam("server", true)->value());
            String username = "";
            if (request->hasParam("username", true))
                urlDecode(request->getParam("username", true)->value());
            String password = "";
            if (request->hasParam("password", true))
                urlDecode(request->getParam("password", true)->value());
            uint port = 1883;
            if (request->hasParam("port", true)) {
                String port_str =
                    urlDecode(request->getParam("port", true)->value());
                port = port_str.toInt();
                if (port == 0)
                    port = 1883;
            }

            mqtt_cfg.printf("%s\n", server.c_str());
            mqtt_cfg.printf("%s\n", username.c_str());
            mqtt_cfg.printf("%s\n", password.c_str());
            mqtt_cfg.printf("%s\n", String(port).c_str());
            mqtt_cfg.flush();
            mqtt_cfg.close();

            if (mqtt_connect(server, username, password, port)) {
                mqtt_enable   = true;
                mqtt_server   = server;
                mqtt_username = username;
                mqtt_password = password;
                mqtt_port     = port;
                request->send(200, "application/json", R"({"status":"ok"})");
                display_draw_mqtt();
            } else {
                String result = R"({"status":"failed","rc":)";
                result += String(mqtt_client.state());
                result += "}";
                mqtt_enable = false;
                request->send(200, "application/json", result);
                display_draw_mqtt();
            }
        });
    xTaskCreate(task_mqtt_loop, "mqtt", 1024 * 8, NULL, 1, NULL);
    xTaskCreate(task_mqtt_publish, "mqtt_publish", 1024 * 8, NULL, 1, NULL);
}

/*!
 * \brief interface to WiFi module for connect to mqtt broker after network
 * available
 */
void mqtt_setup_on_wifi() {
    if (!LITTLEFS.exists("/mqtt.cfg")) {
        mqtt_enable = false;
    } else {
        File mqtt_cfg = LITTLEFS.open("/mqtt.cfg", "r");
        auto server   = mqtt_cfg.readStringUntil('\n');
        auto username = mqtt_cfg.readStringUntil('\n');
        auto password = mqtt_cfg.readStringUntil('\n');
        auto port_str = mqtt_cfg.readStringUntil('\n');
        uint port     = port_str.toInt();

        if (server.length() == 0) {
#if DEBUG
            Serial.println("MQTT configuring file invalid.");
#endif
            mqtt_enable = false;
            display_draw_mqtt();
        } else {
#if DEBUG
            Serial.println("MQTT configuring file valid.");
#endif
            if (mqtt_connect(server, username, password, port)) {
                mqtt_server   = server;
                mqtt_username = username;
                mqtt_password = password;
                mqtt_port     = port;
                mqtt_enable   = true;
                display_draw_mqtt();
            } else {
                mqtt_enable = false;
                display_draw_mqtt();
            }
        }
    }
}
