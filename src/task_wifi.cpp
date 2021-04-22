//
// Created by Shiroko on 2021/4/6.
//

#define LFS_YES_TRACE

#include "clock.h"
#include "display.h"
#include "util.h"
#include "vars.h"
#include <AceRoutine.h>
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <LITTLEFS.h>
#include <WiFi.h>
#include <WiFiGeneric.h>

#define DISCONNECT_MAX_RETRY 5

DNSServer dns_server;

bool wait_for_connect           = false;
bool setup_mode_need_initialize = false;

const IPAddress ap_ip(192, 168, 50, 1);
const char *    ap_ssid  = "AiR_Setup";
const char *    hostname = "air";

wifi_event_id_t event_got_ip, event_connect, event_disconnect;

int               disconnected_retry_count = 0;
SemaphoreHandle_t enable_loop;

void wifi_config_start() {
    setup_mode |= SETUP_MODE_WIFI;
#if DEBUG
    Serial.println("Entering wifi config mode...");
#endif
    WiFi.disconnect();

    WiFi.mode(WIFI_AP_STA);
    WiFi.scanNetworks(true);
    while (WiFi.getMode() != WIFI_AP_STA)
        ;

    WiFi.softAPConfig(ap_ip, ap_ip, IPAddress(255, 255, 255, 0));
    WiFi.softAP(ap_ssid);
    dns_server.setErrorReplyCode(DNSReplyCode::NoError);
    dns_server.start(53, "*", ap_ip);
#if DEBUG
    Serial.printf("Starting Access Point at %s.\n", ap_ssid);
#endif

    display_draw_setup(false);
}

void wifi_config_stop() {
    setup_mode &= ~SETUP_MODE_WIFI;
#if DEBUG
    Serial.println("Leaving wifi config mode.");
#endif
    display_draw_setup(false);
    WiFi.enableAP(false);
    print_log("Leaving WiFi Setup mode.");
}

void wifi_connect() {
    File wifi_cfg = LITTLEFS.open("/wifi.cfg", "r");
    auto ssid     = wifi_cfg.readStringUntil('\n');
    auto password = wifi_cfg.readStringUntil('\n');
    if (ssid.length() == 0 || password.length() < 8) {
#if DEBUG
        Serial.println("WiFi configuring file invalid.");
#endif
        wifi_enable = false;
        wifi_config_start();
    } else {
#if DEBUG
        Serial.println("WiFi configuring file valid.");
#endif
#if DEBUG
        Serial.printf("Connecting to SSID: %s with password: %s.\n",
                      ssid.c_str(), password.c_str());
#endif
        WiFi.mode(WIFI_STA);
        delay(100);
        WiFi.begin(ssid.c_str(), password.c_str());

        wait_for_connect = true;
        wifi_enable      = false;
    }
}

extern void mqtt_setup_on_wifi();

void wifi_setup() {
    // register http server
    http_server.on(
        "/wifi_setting", HTTP_GET, [](AsyncWebServerRequest *request) {
#if DEBUG
            Serial.println("On requesting wifi_setting");
#endif
            if (!LITTLEFS.exists("/wifi_setting.html")) {
                request->send(200, "HTML", "No WiFi Setting HTML...");
                return;
            }
            request->send(
                LITTLEFS, "/wifi_setting.html", String(), false,
                [](const String &var) {
                    if (var == "WIFI_OPTIONS") {
                        int8_t n;
                        String ssid_list = "";
                        if ((n = WiFi.scanComplete()) > 0) {
#if DEBUG
                            Serial.printf("WiFi scan result: %d\n", n);
#endif
                            for (int i = 0; i < n; ++i) {
                                ssid_list += "<option value=\"";
                                ssid_list += WiFi.SSID(i);
                                ssid_list += "\">";
                                ssid_list += WiFi.SSID(i);
                                ssid_list += "</option>";
                            }
                        } else {
#if DEBUG
                            Serial.printf("WiFi scan status: %d\n", n);
#endif
                        }
                        return ssid_list;
                    }
                    return String();
                });
        });
    http_server.on("/api/scan_wifi", HTTP_GET,
                   [](AsyncWebServerRequest *request) {
                       WiFi.scanNetworks(true);
                       int8_t n;
                       while ((n = WiFi.scanComplete()) == WIFI_SCAN_RUNNING)
                           feed_dog();
                       String ssid_list_json = "[";
                       for (int i = 0; i < n; ++i) {
                           ssid_list_json += ("\"" + WiFi.SSID(i) + "\",");
                       }
                       ssid_list_json[ssid_list_json.length() - 1] = ']';
#if DEBUG
                       Serial.println(ssid_list_json);
#endif
                       request->send(200, "application/json", ssid_list_json);
                   });
    http_server.on(
        "/api/set_wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
            if (LITTLEFS.exists("/wifi.cfg")) {
#if DEBUG
                Serial.println("Removing existing file.");
#endif
                LITTLEFS.remove("/wifi.cfg");
            }
#if DEBUG
            Serial.println("Opening wifi.cfg file to write configure.");
#endif
            File wifi_cfg = LITTLEFS.open("/wifi.cfg", "w");
            if (!wifi_cfg) {
#if DEBUG
                Serial.println("Cannot write on LittleFS");
#endif
                request->send(400, "text/plain", "FileSystem corrupted.");
            } else {
#if DEBUG
                Serial.println("Successfully opened file.");
#endif
            }
            if (!request->hasParam("ssid", true) ||
                !request->hasParam("pass", true)) {
                request->send(400, "text/plain", "Arguments invalid.");
                return;
            }
            String ssid = urlDecode(request->getParam("ssid", true)->value());
            String pass = urlDecode(request->getParam("pass", true)->value());

            wifi_cfg.printf("%s\n", ssid.c_str());
            wifi_cfg.printf("%s\n", pass.c_str());
            wifi_cfg.flush();
            wifi_cfg.close();

#if DEBUG
            Serial.printf("Connecting to %s.\n", ssid.c_str());
#endif
            WiFi.begin(ssid.c_str(), pass.c_str());
            request->send(200, "application/json", R"({"status":"ok"})");
            ESP.restart();
        });

    if (fs_usable) {
        if (LITTLEFS.exists("/wifi.cfg")) {
#if DEBUG
            Serial.println("Configuring file detected.");
#endif
            wifi_connect();
        } else {
#if DEBUG
            Serial.println("WiFi configuring file not exists.");
#endif
            wifi_enable = false;
            wifi_config_start();
        }
    } else {
#if DEBUG
        Serial.println("FS is not usable?");
#endif
        wifi_enable = false;
        wifi_config_start();
    }

    event_got_ip = WiFi.onEvent(
        [](WiFiEvent_t event, WiFiEventInfo_t info) {
#if DEBUG
            Serial.println(WiFi.localIP());
#endif
            display_draw_WiFi(WiFi.localIP().toString());
            // wifi connection has been established
            disconnected_retry_count = 0; // reset count
            init_clock();
#if DEBUG
            Serial.println("Connecting to MQTT broker.");
#endif
            mqtt_setup_on_wifi();
        },
        SYSTEM_EVENT_STA_GOT_IP);

    event_connect = WiFi.onEvent(
        [](WiFiEvent_t event, WiFiEventInfo_t info) {
#if DEBUG
            Serial.printf("WiFi connected to %s.\n", WiFi.SSID().c_str());
#endif
            wait_for_connect = false;
            wifi_enable      = true;
            wifi_config_stop();
            if (!MDNS.begin(hostname)) {
#if DEBUG
                Serial.println("Error setting up MDNS!");
#endif
            } else {
#if DEBUG
                Serial.println("MDNS setuped.");
#endif
                MDNS.addService("http", "tcp", 80);
            }
        },
        SYSTEM_EVENT_STA_CONNECTED);

    event_disconnect = WiFi.onEvent(
        [](WiFiEvent_t event, WiFiEventInfo_t info) {
#if DEBUG
            Serial.printf("WiFi disconnected from %s.\n",
                          (const char *)info.disconnected.ssid);
            Serial.printf("Due to : %d\n", info.disconnected.reason);
#endif
            if (disconnected_retry_count <= DISCONNECT_MAX_RETRY &&
                (info.disconnected.reason == WIFI_REASON_AUTH_LEAVE ||
                 info.disconnected.reason == WIFI_REASON_ASSOC_FAIL)) {
                disconnected_retry_count++;
#if DEBUG
                Serial.printf("Trying to reconnect... Times: %d\n",
                              disconnected_retry_count);
#endif
                wifi_connect();
            } else {
                wait_for_connect = false;
                wifi_enable      = false;
                display_clear_wifi(false);
                wifi_config_start();
            }
        },
        SYSTEM_EVENT_STA_DISCONNECTED);
}

[[noreturn]] void task_wifi(void *param) {
    for (;;) {
        if (setup_mode & SETUP_MODE_WIFI) {
            dns_server.processNextRequest();
        }
    }
}