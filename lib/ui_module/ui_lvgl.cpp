#include <lvgl.h>
#include <TFT_eSPI.h>
#include <CST816S.h>
#include "ui_lvgl.h"

#define TFT_HOR_RES   240
#define TFT_VER_RES   240
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

extern TFT_eSPI tft;
extern CST816S touch;

static uint32_t draw_buf[DRAW_BUF_SIZE / 4];
static lv_display_t *disp;

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

    // Example Hello World
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello World, I'm LVGL!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}
