#include "ui_main.h"
#include "TWatch_hal.h"
#include "hardware/blectl.h"


static lv_obj_t *main_window = nullptr;
static lv_obj_t *sub_window = nullptr;
static lv_group_t *g;
LV_IMG_DECLARE(astute_app_64px);

void ui_init(void) {
  g = lv_group_create();
  lv_group_set_default(g);

  lv_indev_t *cur_drv = NULL;
  for (;;) {
    cur_drv = lv_indev_get_next(cur_drv);
    if (!cur_drv) {
      break;
    }
    if (cur_drv->driver->type == LV_INDEV_TYPE_ENCODER) {
      lv_indev_set_group(cur_drv, g);
    }
  }

  //setup BLE
  
  main_window = lv_tileview_create(lv_scr_act());
  lv_obj_set_size(main_window, 240, 240);

  lv_obj_t *img_obj = lv_img_create(main_window);
  lv_img_set_src(img_obj, &astute_app_64px);
  //lv_obj_align_to(img_obj, main_window, LV_ALIGN_CENTER, 0, 0 );
  blectl_setup(img_obj);

}
/*
void ui_create_test_project(lv_obj_t *parent, char *text, lv_event_cb_t event_cb) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_width(btn, lv_pct(100));

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, sub_window);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
}
*/
/*
void ui_send_event_cb(uint8_t btn) {

  switch (btn) {
  case TWATCH_BTN_1:
    if (!get_app_io_test()) {
      lv_obj_set_tile_id(main_window, 0, 0, LV_ANIM_ON);
      lv_obj_clean(sub_window);
    } else {
      app_send_clicked_event(TWATCH_BTN_1);
    }
    break;
  default:
    app_send_clicked_event(btn);
    break;
  }
}
*/