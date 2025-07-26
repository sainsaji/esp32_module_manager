#ifndef UI_LVGL_H
#define UI_LVGL_H

void ui_init();
void my_touchpad_read(struct _lv_indev_t *indev, lv_indev_data_t *data);
extern lv_obj_t* screen_main;
extern void show_wifi_screen();  

#endif
