//
// Created by Shiroko on 2021/4/13.
//

#include "util.h"
#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"
#include <utility>

void feed_dog() {
    // feed dog 0
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE; // write enable
    TIMERG0.wdt_feed     = 1;                   // feed dog
    TIMERG0.wdt_wprotect = 0;                   // write protect
    // feed dog 1
    TIMERG1.wdt_wprotect = TIMG_WDT_WKEY_VALUE; // write enable
    TIMERG1.wdt_feed     = 1;                   // feed dog
    TIMERG1.wdt_wprotect = 0;                   // write protect
}

int temp_goodness(float temp) {
    if ((temp >= 18 && temp <= 25)) {
        return 1;
    }
    if ((temp >= 14 && temp <= 28)) {
        return 2;
    }
    return 3;
}

int humd_goddness(float humd) {
    if (humd >= 40 && humd <= 70) {
        return 1;
    }
    if (humd >= 20 && humd <= 90) {
        return 2;
    }
    return 3;
}

int co2_goddness(unsigned long co2) {
    if (co2 <= 800)
        return 1;
    if (co2 <= 1600)
        return 2;
    return 3;
}

// 0 - smile 1 - straight 2 - bad
int evaluate_air(unsigned long co2, float temp, float humd) {
    int t = temp_goodness(temp);
    int h = humd_goddness(humd);
    int c = co2_goddness(co2);
    if (t == 1 && h == 1 && c == 1)
        return 0;
    if (t <= 2 && h <= 2 && c <= 2)
        return 1;
    return 2;
}

String urlDecode(String input) {
    String s = std::move(input);
    s.replace("%20", " ");
    s.replace("+", " ");
    s.replace("%21", "!");
    s.replace("%22", "\"");
    s.replace("%23", "#");
    s.replace("%24", "$");
    s.replace("%25", "%");
    s.replace("%26", "&");
    s.replace("%27", "\'");
    s.replace("%28", "(");
    s.replace("%29", ")");
    s.replace("%30", "*");
    s.replace("%31", "+");
    s.replace("%2C", ",");
    s.replace("%2E", ".");
    s.replace("%2F", "/");
    s.replace("%2C", ",");
    s.replace("%3A", ":");
    s.replace("%3A", ";");
    s.replace("%3C", "<");
    s.replace("%3D", "=");
    s.replace("%3E", ">");
    s.replace("%3F", "?");
    s.replace("%40", "@");
    s.replace("%5B", "[");
    s.replace("%5C", "\\");
    s.replace("%5D", "]");
    s.replace("%5E", "^");
    s.replace("%5F", "-");
    s.replace("%60", "`");
    return s;
}