#include "config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "astute_app.h"
#include "astute_app_main.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"

#ifdef NATIVE_64BIT
    #include "hardware/tcp/tcp.h"
    #include "utils/logging.h"
#endif

lv_obj_t *astute_app_main_tile = NULL;
lv_obj_t *img_obj = NULL;

LV_IMG_DECLARE(astute_app_64px);
//TcpServer *server;
static bool client_connected_cb( EventBits_t event, void *arg );
static bool client_disconnected_cb( EventBits_t event, void *arg );
static bool new_image_cb( EventBits_t event, void *arg );

void astute_app_main_setup( uint32_t tile_num ) {

    astute_app_main_tile = mainbar_get_tile_obj( tile_num );    
    img_obj = lv_img_create(astute_app_main_tile, NULL);

#ifdef NATIVE_64BIT
    tcp_register_cb(TCPCTL_CONNECT, client_connected_cb, "tcpctl client connected");
    tcp_register_cb(TCPCTL_DISCONNECT, client_disconnected_cb, "tcpctl client disconnected");
    tcp_register_cb(TCPCTL_NEW_IMAGE, new_image_cb, "tcpctl have new image");

#endif
//    server = new TcpServer();
//    server->startListening(3445, img_obj);
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