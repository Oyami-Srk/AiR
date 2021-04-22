//
// Created by Shiroko on 2021/4/14.
//

#ifndef AIR_DISPLAY_H
#define AIR_DISPLAY_H

#include <Arduino.h>

#define LOG_BUFFER_SIZE 4 * 1024 // 4 KB buffer size

void display_draw_WiFi(const String &ip, bool insider = false);
void display_clear_wifi(bool insider = false);
void display_draw_setup(bool insider = false);
void display_draw_bottom_text(const String &s, bool insider = false);
int  print_log(const String &s, bool insider = false);
void display_draw_mqtt(bool insider = false);
void display_draw_transmit(bool enable, bool insider);

#endif // AIR_DISPLAY_H
