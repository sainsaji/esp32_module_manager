#include <lvgl.h>
#include <TFT_eSPI.h>
#include <CST816S.h>
#include "ui_lvgl.h"
#include "wifi_manager.h"
#include "system_info_screen.h"

#define TFT_HOR_RES   240
#define TFT_VER_RES   240
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

extern TFT_eSPI tft;
extern CST816S touch;

static uint32_t draw_buf[DRAW_BUF_SIZE / 4];
static lv_display_t *disp;

lv_obj_t* screen_main;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
    tft.pushColors((uint16_t *)px_map, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1), true);
    tft.endWrite();
    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    if (touch.available()) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touch.data.x;
        data->point.y = touch.data.y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

uint32_t my_tick() {
    return millis();
}

void create_main_screen() {
    screen_main = lv_obj_create(NULL);

    lv_obj_t* title = lv_label_create(screen_main);
    lv_label_set_text(title, "📋 Main Menu");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // WiFi Button
    lv_obj_t* btn_wifi = lv_btn_create(screen_main);
    lv_obj_align(btn_wifi, LV_ALIGN_CENTER, 0, -30);
    lv_obj_add_event_cb(btn_wifi, [](lv_event_t* e) {
        show_wifi_screen();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_wifi = lv_label_create(btn_wifi);
    lv_label_set_text(lbl_wifi, "📡 WiFi Info");

    // System Info Button
    lv_obj_t* btn_sys = lv_btn_create(screen_main);
    lv_obj_align(btn_sys, LV_ALIGN_CENTER, 0, 30);
    lv_obj_add_event_cb(btn_sys, [](lv_event_t* e) {
        lv_scr_load(screen_sysinfo);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_sys = lv_label_create(btn_sys);
    lv_label_set_text(lbl_sys, "🧠 System Info");
}



void ui_init() {
    lv_init();
    lv_tick_set_cb(my_tick);

    disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    create_main_screen();
    create_system_info_screen();

    setup_wifi();            // Initiate WiFi in the background
    lv_scr_load(screen_main); // Start with the main menu screen
}
