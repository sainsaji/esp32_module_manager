#include <lvgl.h>
#include <Arduino.h>
#include "system_info_screen.h"
#include "ui_lvgl.h"
#include "modules.h"

extern lv_obj_t* screen_main;
extern int current_module;

lv_obj_t* screen_sysinfo;

void create_system_info_screen() {
    screen_sysinfo = lv_obj_create(NULL);

    lv_obj_t* title = lv_label_create(screen_sysinfo);
    lv_label_set_text(title, "🧠 System Info");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t* label_model = lv_label_create(screen_sysinfo);
    lv_obj_align(label_model, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_label_set_text_fmt(label_model, "Chip Model: %s", ESP.getChipModel());

    lv_obj_t* label_rev = lv_label_create(screen_sysinfo);
    lv_obj_align(label_rev, LV_ALIGN_TOP_LEFT, 10, 65);
    lv_label_set_text_fmt(label_rev, "Revision: %d", ESP.getChipRevision());

    lv_obj_t* label_cpu = lv_label_create(screen_sysinfo);
    lv_obj_align(label_cpu, LV_ALIGN_TOP_LEFT, 10, 90);
    lv_label_set_text_fmt(label_cpu, "CPU Freq: %d MHz", getCpuFrequencyMhz());

    lv_obj_t* label_flash = lv_label_create(screen_sysinfo);
    lv_obj_align(label_flash, LV_ALIGN_TOP_LEFT, 10, 115);
    lv_label_set_text_fmt(label_flash, "Flash: %d KB", ESP.getFlashChipSize() / 1024);

    lv_obj_t* label_heap = lv_label_create(screen_sysinfo);
    lv_obj_align(label_heap, LV_ALIGN_TOP_LEFT, 10, 140);
    lv_label_set_text_fmt(label_heap, "Free Heap: %d B", ESP.getFreeHeap());

    lv_obj_t* label_block = lv_label_create(screen_sysinfo);
    lv_obj_align(label_block, LV_ALIGN_TOP_LEFT, 10, 165);
    lv_label_set_text_fmt(label_block, "Largest Block: %d B", ESP.getMaxAllocHeap());

    lv_obj_t* label_psram = lv_label_create(screen_sysinfo);
    lv_obj_align(label_psram, LV_ALIGN_TOP_LEFT, 10, 190);
    lv_label_set_text_fmt(label_psram, "Free PSRAM: %d B", ESP.getFreePsram());

    lv_obj_t* label_uptime = lv_label_create(screen_sysinfo);
    lv_obj_align(label_uptime, LV_ALIGN_TOP_LEFT, 10, 215);
    lv_label_set_text_fmt(label_uptime, "Uptime: %lu ms", millis());

    // Optional: current module
    lv_obj_t* label_module = lv_label_create(screen_sysinfo);
    lv_obj_align(label_module, LV_ALIGN_TOP_LEFT, 10, 240);
    if (current_module >= 0) {
        lv_label_set_text_fmt(label_module, "Module: %s", modules[current_module].name);
    } else {
        lv_label_set_text(label_module, "Module: None");
    }

    // Back button
    lv_obj_t* btn_back = lv_btn_create(screen_sysinfo);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn_back, [](lv_event_t* e) {
        lv_scr_load(screen_main);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "🔙 Back");
}
