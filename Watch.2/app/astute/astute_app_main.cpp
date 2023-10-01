#include "config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "astute_app.h"
#include "astute_app_main.h"

#include "TWatch_hal.h"
/*
#ifdef NATIVE_64BIT
    #include "hardware/tcp/tcp.h"
    #include "utils/logging.h"
#else
    #include "hardware/ble/arenabridge.h"
#endif
*/

lv_obj_t *astute_app_main_tile = NULL;
lv_obj_t *img_obj = NULL;


lv_obj_t *text_label = NULL;
lv_style_t debug_style;

//TcpServer *server;
static bool client_connected_cb( EventBits_t event, void *arg );
static bool client_disconnected_cb( EventBits_t event, void *arg );
static bool new_image_cb( EventBits_t event, void *arg );
static bool have_debug_message_cb( EventBits_t event, void *arg );

void astute_app_main_setup( uint32_t tile_num ) {
/*
    astute_app_main_tile = mainbar_get_tile_obj( tile_num );    
    img_obj = lv_img_create(astute_app_main_tile, NULL);
    lv_img_set_src(img_obj, &astute_app_64px);


    // debug label
    lv_style_copy(&debug_style, ws_get_label_style());
    lv_style_set_text_color(&debug_style, LV_STATE_DEFAULT, LV_COLOR_GREEN);
    lv_style_set_bg_opa(&debug_style, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_style_set_text_opa(&debug_style, LV_STATE_DEFAULT, LV_OPA_50);
	lv_style_set_pad_left(&debug_style, LV_STATE_DEFAULT, 3);
	lv_style_set_pad_right(&debug_style, LV_STATE_DEFAULT, 3);

    text_label = lv_label_create(astute_app_main_tile, NULL);

    lv_label_set_long_mode(text_label, LV_LABEL_LONG_CROP);
    lv_obj_add_style(text_label, LV_OBJ_PART_MAIN, &debug_style);
    lv_obj_align(text_label, astute_app_main_tile, LV_ALIGN_CENTER, 0, 0 );
	lv_obj_set_size(text_label, lv_disp_get_hor_res( NULL ), 240 );

    


#ifdef NATIVE_64BIT
    tcp_register_cb(TCPCTL_CONNECT, client_connected_cb, "tcpctl client connected");
    tcp_register_cb(TCPCTL_DISCONNECT, client_disconnected_cb, "tcpctl client disconnected");
    tcp_register_cb(TCPCTL_NEW_IMAGE, new_image_cb, "tcpctl have new image");
    tcp_register_cb(TCPCTL_DEBUG_MESSAGE, have_debug_message_cb, "tcpctl client debug message");
#else
    arenabridge_register_cb(BLECTL_DEBUG_MESSAGE, have_debug_message_cb, "ble client debug message");
    arenabridge_register_cb(BLECTL_NEW_IMAGE, new_image_cb, "ble client have new image");

#endif
*/
}


static bool client_connected_cb( EventBits_t event, void *arg ) {
    log_i("new client connected");
    return true;
}

static bool client_disconnected_cb( EventBits_t event, void *arg ) {
    return true;
}

static bool new_image_cb( EventBits_t event, void *arg ) {
    log_i("have new image ====");
    lv_img_set_src(img_obj, arg);
    return true;
}

static bool have_debug_message_cb(EventBits_t event, void *arg) {
    lv_label_set_text(text_label, (const char *)arg);
    return true;
}