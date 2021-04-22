//
// Created by Shiroko on 2021/4/9.
//
#define USE_FAST_PINIO

#include "PMS5003T.h"
#include "display.h"
#include "util.h"
#include "vars.h"
#include <Arduino.h>
#include <SPI.h>
#include <TFT_22_ILI9225.h>
#include <ctime>

#define TFT_RST 26 // IO 26
#define TFT_RS  27 // IO 25
#define TFT_CLK 12 // HSPI-SCK
//#define TFT_SDO 12  // HSPI-MISO
#define TFT_SDI 14 // HSPI-MOSI
#define TFT_CS  25 // HSPI-SS0
#define TFT_LED 13 // 0 if wired to +5V directly
//#define LIGHT_RESISTOR 34

SPIClass hspi(HSPI);

#define TFT_BRIGHTNESS         100
#define TFT_BRIGNTNESS_CHANNEL 1

TFT_22_ILI9225 tft =
    TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);

#include "image.c"

bool display_task_running   = false;
int  ip_line_pos            = 0;
bool status_bar_need_redraw = false;

void display_draw_WiFi(const String &ip, bool insider) {
    if (!display_task_running || insider) {
        tft.drawBitmap(0, 1, bitmap_wifi, 16, 16, COLOR_WHITE);
        tft.setFont(Terminal6x8);
        tft.drawText(66 + 5, 5, ip, COLOR_AZUR);
        int width = tft.getTextWidth(ip) + 66 + 10;
        tft.drawLine(width, 1, width, 15, COLOR_DARKGREY);
        ip_line_pos = width;
    } else
        status_bar_need_redraw = true;
}

void display_draw_mqtt(bool insider) {
    if (!display_task_running || insider)
        tft.drawBitmap(16, 1, bitmap_mqtt, 16, 16,
                       mqtt_enable ? COLOR_WHITE : COLOR_BLACK);
    else
        status_bar_need_redraw = true;
}

void display_draw_transmit(bool enable, bool insider) {
    if (!display_task_running || insider)
        tft.drawBitmap(48, 1, bitmap_transmissing, 16, 16,
                       enable ? COLOR_WHITE : COLOR_BLACK);
    else
        status_bar_need_redraw = true;
}

void display_set_brigntness(int8_t brigntness) {
    ledcWrite(TFT_BRIGNTNESS_CHANNEL, brigntness);
}

void display_draw_statusbar() {
    tft.drawLine(1, 16, tft.maxX(), 16, COLOR_WHITE);
    tft.drawLine(65, 1, 65, 15, COLOR_DARKGREY);

    if (wifi_enable) {
        display_draw_WiFi(WiFi.localIP().toString(), true);
    }
    if (mqtt_enable)
        tft.drawBitmap(16, 1, bitmap_mqtt, 16, 16, COLOR_WHITE);
    if (setup_mode)
        tft.drawBitmap(32, 1, bitmap_setup, 16, 16, COLOR_WHITE);
    /*
    if (false)
        tft.drawBitmap(48, 1, bitmap_transmissing, 16, 16, COLOR_WHITE);
        */
    tft.drawBitmap(tft.maxX() - 16, 1, bitmap_logo, 16, 16, COLOR_GREEN);
}

const static uint8_t *sidebar_index[] = {bitmap_home, bitmap_data, bitmap_info,
                                         bitmap_restart};

const static size_t sidebar_size           = 4;
const static size_t max_bottom_text_length = (ILI9225_LCD_HEIGHT - 13 - 1) / 6;

int            last_day        = 88;
int            last_min        = 88;
int            last_hour       = 88;
int            last_sec        = 88;
unsigned long  last_co2        = 9999;
uint8_t *      last_co2_status = NULL;
PMS5003T::DATA last_pms_data;
int            last_emotion      = -1;
int            last_display_page = 9999;
bool           page_turned       = false;

extern unsigned long  co2;      // co2
extern PMS5003T::DATA pms_data; // pms data
extern PMS5003T       pms;      // pms

void display_draw_sidebar(int current = 0) {
    if (current >= sidebar_size)
        return;
    for (int i = 0; i < sidebar_size; i++) {
        if (current != i)
            tft.drawBitmap(tft.maxX() - 17, 32 + i * 16, sidebar_index[i], 16,
                           16, COLOR_DARKGREY, COLOR_BLACK);
        else
            tft.drawBitmap(tft.maxX() - 17, 32 + i * 16, sidebar_index[i], 16,
                           16, COLOR_BLUE, COLOR_WHITE);
    }
}

// dont use \n in the string.
void display_draw_bottom_text(const String &s, bool insider) {
    _currentFont original_font = tft.getFont();
    tft.setFont(Terminal6x8);
    tft.fillRectangle(13, tft.maxY() - 10, tft.maxX() - 2, tft.maxY() - 2,
                      COLOR_BLACK);
    tft.drawText(1, tft.maxY() - 9, "> ", COLOR_DARKGREEN);
    if (max_bottom_text_length >= s.length())
        tft.drawText(13, tft.maxY() - 9, s, COLOR_WHITE);
    else {
        char buffer[max_bottom_text_length + 1] = {0};
        memcpy(buffer, s.c_str(), max_bottom_text_length);
        tft.drawText(13, tft.maxY() - 9, buffer, COLOR_WHITE);
    }
    if (original_font.font)
        tft.setFont(original_font.font);
}

const static uint8_t PROGMEM smile_y[] = {0, 1, 2, 3, 3, 3, 4, 4,
                                          4, 3, 3, 3, 2, 1, 0};

// 0=smile 1=straight 2=frown
void draw_emotion_icon_32x32(int type, int x, int y) {
    tft.drawCircle(x + 16, y + 16, 15, COLOR_GREEN);
    tft.drawCircle(x + 16, y + 16, 16, COLOR_GREEN);
    tft.fillCircle(x + 10, y + 16 - 5, 2, COLOR_WHITE);
    tft.fillCircle(x + 22, y + 16 - 5, 2, COLOR_WHITE);

    switch (type) {
    case 0:
        for (int i = 0; i <= 14; i++) {
            tft.fillCircle(x + i + 9, y + 16 + 5 + smile_y[i], 2,
                           COLOR_LIGHTBLUE);
        }
        break;
    case 1:
        for (int i = 9; i <= 23; i++) {
            tft.fillCircle(x + i, y + 16 + 5, 2, COLOR_LIGHTBLUE);
        }
        break;
    case 2:
        for (int i = 0; i <= 14; i++) {
            tft.fillCircle(x + i + 9, y + 16 + 5 + 2 - smile_y[i], 2,
                           COLOR_LIGHTBLUE);
        }
        break;
    default:
        break;
    }
}

void display_resetup() {
    tft.drawRectangle(0, 0, tft.maxX() - 1, tft.maxY() - 1, COLOR_GRAY);

    tft.drawLine(1, tft.maxY() - 11, tft.maxX() - 2, tft.maxY() - 11,
                 COLOR_GRAY);
    tft.drawLine(1, 16, tft.maxX() - 2, 16, COLOR_WHITE);
    tft.drawLine(65, 1, 65, 15, COLOR_DARKGREY);
    tft.drawBitmap(tft.maxX() - 16, 1, bitmap_logo, 16, 16, COLOR_GREEN);
    display_draw_statusbar();
}

void display_setup() {
    memset(&last_pms_data, 0, sizeof(PMS5003T::DATA));
    hspi.begin(TFT_CLK, -1, TFT_SDI, TFT_CS);
    tft.begin(hspi);
    tft.setOrientation(3);
    ledcSetup(TFT_BRIGNTNESS_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_LED, TFT_BRIGNTNESS_CHANNEL);
    display_set_brigntness(100);

    display_draw_bottom_text("Hello world!");

    display_resetup();

    pinMode(34, INPUT_PULLDOWN);
}

void update_display();

[[noreturn]] void task_display(void *param) {
    static portTickType       last_wake_time = xTaskGetTickCount();
    const static portTickType task_freq =
        pdMS_TO_TICKS(1000); // every 1sec update a screen
    for (;;) {
        display_task_running = true;
        update_display();
        display_task_running = false;
        vTaskDelayUntil(&last_wake_time, task_freq);
    }
}

// used by other task

void display_clear_wifi(bool insider) {
    if (!display_task_running || insider) {
        tft.fillRectangle(1, 1, 15, 15, COLOR_BLACK);
        if (ip_line_pos != 0) {
            tft.fillRectangle(66 + 5, 1, 66 + 10 + ip_line_pos, 15,
                              COLOR_BLACK);
        }
    } else
        status_bar_need_redraw = true;
}

void display_draw_setup(bool insider) {
    if (!display_task_running || insider) {
        if (setup_mode)
            tft.drawBitmap(32, 1, bitmap_setup, 16, 16, COLOR_WHITE);
        else
            tft.fillRectangle(32, 1, 32 + 16, 15, COLOR_BLACK);
    } else
        status_bar_need_redraw = true;
}

int print_log(const String &s, bool insider) {
    if (!display_task_running || insider)
        display_draw_bottom_text(s, insider);
    return 0;
}

// usable area:
// x0: 1, y0: 17;

#define HOUR_POSITION  10
#define MIN_POSITION   (10 + 16 * 3 - 8)
#define SEC_POSITION   (10 + 16 * 6 - 15)
#define SEP_1_POSITION (10 + 16 * 2 + 2)
#define SEP_2_POSITION (10 + 16 * 5 - 7)

void init_home_screen() {
    tft.setFont(Terminal12x16);
    tft.drawBitmap(10, 85, bitmap_wendu, 32, 16, COLOR_WHITE);
    tft.drawBitmap(10, 102, bitmap_shidu, 32, 16, COLOR_WHITE);
    tft.drawBitmap(10, 118, bitmap_pm25, 40, 16, COLOR_WHITE);
    tft.drawBitmap(10 + 40 + 60, 85, bitmap_cel, 16, 16, COLOR_WHITE);
    tft.drawText(10 + 40 + 60 + 2, 102, "%", COLOR_WHITE);
    tft.drawBitmap(10 + 40 + 60, 118, bitmap_ugm3, 43, 16, COLOR_WHITE);
    tft.drawBitmap(10, 58, bitmap_co2, 54, 21, COLOR_WHITE);
}

void home_screen() {
    // print time and date
    struct tm time_info;
    if (getLocalTime(&time_info)) {
        char num_buf[4];
        // successfully obtained time
        tft.setFont(Trebuchet_MS16x21);
        // draw hour
        if (time_info.tm_hour != last_hour || page_turned) {
            sprintf(num_buf, "%02d", last_hour);
            tft.drawText(HOUR_POSITION, 20, num_buf, COLOR_BLACK);
            last_hour = time_info.tm_hour;
            sprintf(num_buf, "%02d", last_hour);
            tft.drawText(HOUR_POSITION, 20, num_buf, COLOR_WHITE);
        }
        // draw min
        if (time_info.tm_min != last_min || page_turned) {
            sprintf(num_buf, "%02d", last_min);
            tft.drawText(MIN_POSITION, 20, num_buf, COLOR_BLACK);
            last_min = time_info.tm_min;
            sprintf(num_buf, "%02d", last_min);
            tft.drawText(MIN_POSITION, 20, num_buf, COLOR_WHITE);
        }
        // draw second
        sprintf(num_buf, "%02d", last_sec);
        tft.drawText(SEC_POSITION, 20, num_buf, COLOR_BLACK);
        last_sec = time_info.tm_sec;
        sprintf(num_buf, "%02d", last_sec);
        tft.drawText(SEC_POSITION, 20, num_buf, COLOR_WHITE);

        // draw separator
        uint16_t sep_color =
            (time_info.tm_sec % 2 == 0) ? COLOR_BLACK : COLOR_WHITE;
        tft.drawText(SEP_1_POSITION, 20 - 2, ":\0", sep_color);
        tft.drawText(SEP_2_POSITION, 20 - 2, ":\0", sep_color);

        // draw date
        if (time_info.tm_mday != last_day || page_turned) {
            last_day = time_info.tm_mday;
            tft.setFont(Terminal6x8);
            char time_buffer[16];
            strftime(time_buffer, 16, "%Y/%m/%d", &time_info);
            tft.fillRectangle(4 + 20 + 7, 20 + 21 + 1, 4 + 20 + 6 * 10 + 7,
                              20 + 21 + 1 + 8, COLOR_BLACK);
            tft.drawText(4 + 20 + 7, 20 + 21 + 1, time_buffer, COLOR_GRAY);
        }
    }
    // print co2
    unsigned long local_co2 = 0;
    local_co2               = co2;
    if (local_co2 != last_co2 || page_turned) {
        int16_t        co2_color = COLOR_WHITE;
        const uint8_t *status    = bitmap_zhengchang;
        if (local_co2 <= 500) {
            co2_color = COLOR_GREEN;
            status    = bitmap_lianghao;
        } else if (local_co2 >= 1000) {
            co2_color = COLOR_ORANGE;
            status    = bitmap_luegao;
        } else if (local_co2 >= 2000) {
            co2_color = COLOR_RED;
            status    = bitmap_guogao;
        } else if (local_co2 >= 3000) {
            co2_color = COLOR_DARKRED;
            status    = bitmap_chaogao;
        }
        tft.setFont(Trebuchet_MS16x21);
        char buffer[16];
        sprintf(buffer, "%lu", last_co2);
        tft.drawText(64, 58, buffer, COLOR_BLACK);
        last_co2 = local_co2;
        sprintf(buffer, "%lu", local_co2);
        tft.drawText(64, 58, buffer, co2_color);
        tft.drawRectangle(64 + 64 + 10, 58 - 1, 64 + 64 + 10 + 49, 58 + 21 + 1,
                          co2_color);
        if (last_co2_status) {
            tft.drawBitmap(64 + 64 + 10, 58, last_co2_status, 48, 21,
                           COLOR_BLACK);
        }
        tft.drawBitmap(64 + 64 + 10, 58, status, 48, 21, co2_color);
        last_co2_status = (uint8_t *)status;
    }

    // print pms data
    PMS5003T::DATA local_pms_data{};
    memset(&local_pms_data, 0, sizeof(PMS5003T::DATA));
    float temp = 0, humd = 0;
    if (xSemaphoreTake(mutex_pms, pdMS_TO_TICKS(500)) == pdTRUE) {
        memcpy(&local_pms_data, &pms_data, sizeof(PMS5003T::DATA));
        xSemaphoreGive(mutex_pms);
        temp = local_pms_data.TEMP / 10.0f;
        humd = local_pms_data.HUMD / 10.0f;
        tft.setFont(Terminal12x16);

        if (local_pms_data.TEMP != last_pms_data.TEMP || page_turned) {
            tft.drawText(10 + 40, 85, String(last_pms_data.TEMP / 10.0f),
                         COLOR_BLACK);
            tft.drawText(10 + 40, 85, String(local_pms_data.TEMP / 10.0f),
                         COLOR_WHITE);
        }
        if (local_pms_data.HUMD != last_pms_data.HUMD || page_turned) {
            tft.drawText(10 + 40, 102, String(last_pms_data.HUMD / 10.0f),
                         COLOR_BLACK);
            tft.drawText(10 + 40, 102, String(local_pms_data.HUMD / 10.0f),
                         COLOR_WHITE);
        }
        if (local_pms_data.PM_AE_UG_2_5 != last_pms_data.PM_AE_UG_2_5 ||
            page_turned) {
            tft.drawText(10 + 40 + 8, 118, String(last_pms_data.PM_AE_UG_2_5),
                         COLOR_BLACK);
            tft.drawText(10 + 40 + 8, 118, String(local_pms_data.PM_AE_UG_2_5),
                         COLOR_WHITE);
        }
        memcpy(&last_pms_data, &local_pms_data, sizeof(PMS5003T::DATA));
    }
    const int emo_position_neg_x = 50;
    const int emo_position_neg_y = 50;
    if (humd != 0 && temp != 0 && co2 != 0) {
        int emo = evaluate_air(co2, temp, humd);
        if (emo != last_emotion || page_turned) {
            tft.fillCircle(tft.maxX() - emo_position_neg_x + 16,
                           tft.maxY() - emo_position_neg_y + 16, 14,
                           COLOR_BLACK);
            last_emotion = emo;
        }
        draw_emotion_icon_32x32(emo, tft.maxX() - emo_position_neg_x,
                                tft.maxY() - emo_position_neg_y);
    }
}

void init_detail_screen() {
    tft.drawBitmap(4, 17, bitmap_details, 200, 144, COLOR_WHITE);
}

#define PMS_DETAIL_PRINT(x, y, d)                                              \
    if (last_pms_data.d != local_pms_data.d || page_turned) {                  \
        tft.drawText((x) + 4, (y) + 17, String(last_pms_data.d), COLOR_BLACK); \
        tft.drawText((x) + 4, (y) + 17, String(local_pms_data.d),              \
                     COLOR_WHITE);                                             \
    }

void detail_screen() {
    PMS5003T::DATA local_pms_data{};
    memset(&local_pms_data, 0, sizeof(PMS5003T::DATA));
    if (xSemaphoreTake(mutex_pms, pdMS_TO_TICKS(500)) == pdTRUE) {
        memcpy(&local_pms_data, &pms_data, sizeof(PMS5003T::DATA));
        xSemaphoreGive(mutex_pms);
        tft.setFont(Terminal11x16);
        PMS_DETAIL_PRINT(26, 27, PM_AE_UG_1_0)
        PMS_DETAIL_PRINT(75, 27, PM_AE_UG_2_5)
        PMS_DETAIL_PRINT(135, 27, PM_AE_UG_10_0)

        PMS_DETAIL_PRINT(26, 56, PM_FE_UG_1_0)
        PMS_DETAIL_PRINT(75, 56, PM_FE_UG_2_5)
        PMS_DETAIL_PRINT(135, 56, PM_FE_UG_10_0)

        tft.setFont(Terminal6x8);

        PMS_DETAIL_PRINT(43, 126, ABOVE_0dot3_um)
        PMS_DETAIL_PRINT(83, 126, ABOVE_0dot5_um)
        PMS_DETAIL_PRINT(124, 126, ABOVE_1dot0_um)
        PMS_DETAIL_PRINT(164, 126, ABOVE_2dot5_um)
    }
}

void info_screen() {}

void update_display() {
    if (display_page != last_display_page) {
        tft.clear();
        display_resetup();
        display_draw_sidebar(display_page);
        last_display_page = display_page;
        tft.fillRectangle(1, 17, tft.maxX() - 17, tft.maxY() - 12, COLOR_BLACK);
        switch (display_page) {
        case 0:
            init_home_screen();
            display_draw_bottom_text("Switch to home screen.");
            break;
        case 1:
            init_detail_screen();
            display_draw_bottom_text("Switch to details screen.");
            break;
        case 2:
            tft.drawBitmap(4, 17, bitmap_info_src, 200, 144, COLOR_WHITE);
            display_draw_bottom_text("Switch to info screen.");
            break;
        case 3:
            display_draw_bottom_text("Reboot, Are you sure?");
            break;
        default:
            break;
        }
        page_turned = true;
    }
    if (status_bar_need_redraw) {
        display_draw_statusbar();
        status_bar_need_redraw = false;
    }
    switch (display_page) {
    case 0:
        home_screen();
        break;
    case 1:
        detail_screen();
        break;
    case 2:
        info_screen();
        break;
    case 3:
        break;
    default:
        break;
    }
    page_turned = false;
}
