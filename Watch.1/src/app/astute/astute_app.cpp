#include "config.h"

#include "astute_app.h"
#include "astute_app_main.h"

#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/app.h"
#include "gui/widget.h"

uint32_t astute_app_main_tile_num;

// app icon
icon_t *astute_app = NULL;

// declare you images or fonts you need
LV_IMG_DECLARE(astute_app_64px);

/*
 * setup routine for example app
 */
void astute_app_setup( void ) {
    astute_app_main_tile_num = mainbar_add_app_tile( 2, 1, "Astute app" );
    astute_app = app_register( "Astute", &astute_app_64px, enter_astute_app_event_cb );
    astute_app_main_setup( astute_app_main_tile_num );
}

/*
 *
 */
uint32_t astute_app_get_app_main_tile_num( void ) {
    return( astute_app_main_tile_num );
}

/*
 *
 */
void enter_astute_app_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_to_tilenumber( astute_app_main_tile_num, LV_ANIM_OFF, true );
                                        app_hide_indicator( astute_app );
                                        break;
    }
}

/*
 *
 */
void exit_astute_app_main_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_back();
                                        break;
    }
}