/****************************************************************************
 *   Tu May 22 21:23:51 2020
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include <time.h>

#include "config.h"

#include "gui/mainbar/mainbar.h"
#include "gui/widget_styles.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/note_tile/note_tile.h"
#include "gui/mainbar/setup_tile/setup_tile.h"
#include "gui/widget_styles.h"

#include "hardware/timesync.h"
#include "hardware/powermgm.h"
#include "hardware/pmu.h"
#include "hardware/sensor.h"
#include "hardware/blectl.h"
#include "hardware/wifictl.h"

#include "utils/alloc.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
#else
    #include <Arduino.h>
#endif

static bool maintile_init = false;

static lv_obj_t *main_cont = NULL;
static lv_obj_t *clock_cont = NULL;
static lv_obj_t *info_cont = NULL;
static lv_obj_t *batterylabel = NULL;
static lv_obj_t *batteryicon = NULL;
static lv_obj_t *wifiicon = NULL;
static lv_obj_t *bluetoothicon = NULL;
static lv_obj_t *timelabel = NULL;
static lv_obj_t *infolabel = NULL;
static lv_obj_t *templabel = NULL;
static lv_obj_t *datelabel = NULL;

uint32_t main_tile_num;

static lv_style_t *style;
static lv_style_t timestyle;
static lv_style_t datestyle;
static lv_style_t infostyle;
static lv_style_t tempstyle;
static lv_style_t iconstyle;

icon_t widget_entry[ MAX_WIDGET_NUM ];

LV_FONT_DECLARE(Ubuntu_144px);
LV_FONT_DECLARE(Ubuntu_72px);
LV_FONT_DECLARE(Ubuntu_48px);
LV_FONT_DECLARE(Ubuntu_32px);
LV_FONT_DECLARE(Ubuntu_16px);

#if defined( M5PAPER )
    lv_font_t *time_font = &Ubuntu_144px;
    lv_font_t *date_font = &Ubuntu_48px;
    lv_font_t *info_font = &Ubuntu_32px;
    lv_font_t *temp_font = &Ubuntu_32px;
    lv_font_t *icon_font = &Ubuntu_16px;
#else
    lv_font_t *time_font = &Ubuntu_72px;
    lv_font_t *date_font = &Ubuntu_16px;
    lv_font_t *info_font = &Ubuntu_16px;    
    lv_font_t *temp_font = &Ubuntu_16px;    
    lv_font_t *icon_font = &Ubuntu_16px;
#endif

lv_task_t * main_tile_task;

static void main_tile_update_task( lv_task_t * task );
static bool mainbar_blectl_event_cb( EventBits_t event, void *arg );
static bool mainbar_wifictl_event_cb( EventBits_t event, void *arg );
static bool mainbar_pmu_event_cb( EventBits_t event, void *arg );
static bool main_tile_powermgm_event_cb( EventBits_t event, void *arg );
static bool main_tile_time_update_ebent_cb( EventBits_t event, void *arg );
static bool main_tile_style_event_cb( EventBits_t event, void *arg );
static bool main_tile_button_event_cb( EventBits_t event, void *arg );
static bool main_tile_sensor_event_cb( EventBits_t event, void *arg );

void main_tile_setup( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( maintile_init ) {
        log_e("maintile already initialized");
        return;
    }

    main_tile_num = mainbar_add_tile( 0, 0, "main tile", ws_get_mainbar_style() );
    main_cont = mainbar_get_tile_obj( main_tile_num );
    style = ws_get_mainbar_style();

    lv_style_copy( &timestyle, style);
    lv_style_set_text_font( &timestyle, LV_STATE_DEFAULT, time_font );

    lv_style_copy( &datestyle, style);
    lv_style_set_text_font( &datestyle, LV_STATE_DEFAULT, date_font );

    lv_style_copy( &infostyle, style);
    lv_style_set_text_font( &infostyle, LV_STATE_DEFAULT, info_font );

    lv_style_copy( &tempstyle, style);
    lv_style_set_text_font( &tempstyle, LV_STATE_DEFAULT, temp_font );

    lv_style_copy( &iconstyle, style);
    lv_style_set_text_font( &iconstyle, LV_STATE_DEFAULT, icon_font );

    clock_cont = mainbar_obj_create( main_cont );
    lv_obj_set_size( clock_cont, lv_disp_get_hor_res( NULL ) , lv_disp_get_ver_res( NULL ) / 2 );
    lv_obj_add_style( clock_cont, LV_OBJ_PART_MAIN, style );
    lv_obj_align( clock_cont, main_cont, LV_ALIGN_CENTER, 0, 0 );
    
    timelabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(timelabel, "00:00");
    lv_obj_reset_style_list( timelabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( timelabel, LV_OBJ_PART_MAIN, &timestyle );
    lv_obj_align(timelabel, NULL, LV_ALIGN_CENTER, 0, 0);

    datelabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(datelabel, "1.Jan 1970");
    lv_obj_reset_style_list( datelabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( datelabel, LV_OBJ_PART_MAIN, &datestyle );
    lv_obj_align( datelabel, clock_cont, LV_ALIGN_IN_BOTTOM_MID, 0, 0 );

    infolabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(infolabel, "battery: n/a");
    lv_obj_reset_style_list( infolabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( infolabel, LV_OBJ_PART_MAIN, &infostyle );
    lv_obj_align( infolabel, datelabel, LV_ALIGN_OUT_TOP_MID, 0, 0 );

    templabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(templabel, "temp/humidity: n/a");
    lv_obj_reset_style_list( templabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( templabel, LV_OBJ_PART_MAIN, &tempstyle );
    lv_obj_align( templabel, infolabel, LV_ALIGN_OUT_TOP_MID, 0, 0 );
    
    if ( !sensor_get_available() )
        lv_obj_set_hidden( templabel, true );

    main_tile_update_time( true );

    for ( int widget = 0 ; widget < MAX_WIDGET_NUM ; widget++ ) {
        widget_entry[ widget ].active = false;

        widget_entry[ widget ].icon_cont = mainbar_obj_create( main_cont );
        lv_obj_reset_style_list( widget_entry[ widget ].icon_cont, LV_OBJ_PART_MAIN );
        lv_obj_add_style( widget_entry[ widget ].icon_cont, LV_OBJ_PART_MAIN, style );
        lv_obj_set_size( widget_entry[ widget ].icon_cont, WIDGET_X_SIZE, WIDGET_Y_SIZE );
        lv_obj_set_hidden( widget_entry[ widget ].icon_cont, true );
        // create app label
        widget_entry[ widget ].label = lv_label_create( widget_entry[ widget ].icon_cont , NULL );
        mainbar_add_slide_element( widget_entry[ widget ].label);
        lv_obj_reset_style_list( widget_entry[ widget ].label, LV_OBJ_PART_MAIN );
        lv_obj_add_style( widget_entry[ widget ].label, LV_OBJ_PART_MAIN, &iconstyle );
        lv_obj_set_size( widget_entry[ widget ].label, WIDGET_X_SIZE, WIDGET_LABEL_Y_SIZE );
        lv_obj_align( widget_entry[ widget ].label , widget_entry[ widget ].icon_cont, LV_ALIGN_IN_BOTTOM_MID, 0, 0 );
        // create app label
        widget_entry[ widget ].ext_label = lv_label_create( widget_entry[ widget ].icon_cont , NULL );
        mainbar_add_slide_element( widget_entry[ widget ].ext_label);
        lv_obj_reset_style_list( widget_entry[ widget ].ext_label, LV_OBJ_PART_MAIN );
        lv_obj_add_style( widget_entry[ widget ].ext_label, LV_OBJ_PART_MAIN, &iconstyle );
        lv_obj_set_size( widget_entry[ widget ].ext_label, WIDGET_X_SIZE, WIDGET_LABEL_Y_SIZE );
        lv_obj_align( widget_entry[ widget ].ext_label , widget_entry[ widget ].label, LV_ALIGN_OUT_TOP_MID, 0, 0 );
        // create img and indicator
        widget_entry[ widget ].icon_img = lv_imgbtn_create( widget_entry[ widget ].icon_cont , NULL );
        widget_entry[ widget ].icon_indicator = lv_img_create( widget_entry[ widget ].icon_cont, NULL );
        // hide all
        lv_obj_set_hidden( widget_entry[ widget ].icon_cont, true );
        lv_obj_set_hidden( widget_entry[ widget ].icon_img, true );
        lv_obj_set_hidden( widget_entry[ widget ].icon_indicator, true );
        lv_obj_set_hidden( widget_entry[ widget ].label, true );
        lv_obj_set_hidden( widget_entry[ widget ].ext_label, true );
    }

    info_cont = mainbar_obj_create( clock_cont );
    lv_obj_set_size( info_cont, lv_disp_get_hor_res( NULL ) , 16 );
    lv_obj_add_style( info_cont, LV_OBJ_PART_MAIN, style );
    lv_obj_align( info_cont, clock_cont, LV_ALIGN_IN_TOP_MID, 0, 8 );

    batteryicon = lv_img_create( info_cont , NULL);
    lv_img_set_src( batteryicon, LV_SYMBOL_BATTERY_FULL );
    lv_obj_align( batteryicon, info_cont, LV_ALIGN_CENTER, 0, 0 );

    wifiicon = lv_img_create( info_cont , NULL);
    lv_img_set_src( wifiicon, LV_SYMBOL_WIFI );
    lv_obj_align( wifiicon, batteryicon, LV_ALIGN_OUT_LEFT_MID, -THEME_PADDING, 0 );

    bluetoothicon = lv_img_create( info_cont , NULL);
    lv_img_set_src( bluetoothicon, LV_SYMBOL_BLUETOOTH );
    lv_obj_align( bluetoothicon, wifiicon, LV_ALIGN_OUT_LEFT_MID, -THEME_PADDING, 0 );

    batterylabel = lv_label_create( info_cont , NULL);
    lv_label_set_text(batterylabel, "100%" );
    lv_obj_reset_style_list( batterylabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( batterylabel, LV_OBJ_PART_MAIN, &datestyle );
    lv_obj_align(batterylabel, batteryicon, LV_ALIGN_OUT_RIGHT_MID, THEME_PADDING, 0);

    lv_obj_set_style_local_image_recolor( wifiicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY );
    lv_obj_set_style_local_image_recolor( bluetoothicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY );
    lv_obj_set_hidden( wifiicon, true );
    lv_obj_set_hidden( bluetoothicon, true );

    main_tile_task = lv_task_create( main_tile_update_task, 500, LV_TASK_PRIO_MID, NULL );

    pmu_register_cb( PMUCTL_STATUS, mainbar_pmu_event_cb, "mainbar pmu");
    blectl_register_cb( BLECTL_CONNECT | BLECTL_DISCONNECT | BLECTL_ON | BLECTL_OFF, mainbar_blectl_event_cb, "mainbar bluetooth" );
    wifictl_register_cb( WIFICTL_CONNECT | WIFICTL_DISCONNECT | WIFICTL_OFF | WIFICTL_ON, mainbar_wifictl_event_cb, "mainbar wifi" );
    powermgm_register_cb( POWERMGM_WAKEUP , main_tile_powermgm_event_cb, "main tile time update" );
    timesync_register_cb( TIME_SYNC_UPDATE, main_tile_time_update_ebent_cb, "main tile time sync" );
    sensor_register_cb( SENSOR_TEMPERATURE | SENSOR_RELHUMIDITY, main_tile_sensor_event_cb, "main tile sensor" );
    mainbar_add_tile_button_cb( main_tile_num, main_tile_button_event_cb );
    styles_register_cb( STYLE_CHANGE, main_tile_style_event_cb, "main tile style event" );

    #ifndef ROUND_DISPLAY
        lv_obj_set_hidden( info_cont, true );
    #endif
    #ifndef M5PAPER
        lv_obj_set_hidden( infolabel, true );
    #endif

    maintile_init = true;
}

static bool mainbar_pmu_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case PMUCTL_STATUS: {
                int32_t pmu_info = *(int32_t*)arg;
                int32_t percent = pmu_info & PMUCTL_STATUS_PERCENT;
                wf_label_printf( batterylabel, "%d%%", percent );
                lv_obj_align(batterylabel, batteryicon, LV_ALIGN_OUT_RIGHT_MID, THEME_PADDING, 0);
                if( pmu_info & PMUCTL_STATUS_CHARGING ) {
                    lv_obj_set_style_local_image_recolor( batteryicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN );
                    lv_img_set_src( batteryicon, LV_SYMBOL_CHARGE );
                }
                else {
                    lv_obj_set_style_local_image_recolor( batteryicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE );
                    if ( percent >= 75 ) { 
                        lv_img_set_src( batteryicon, LV_SYMBOL_BATTERY_FULL );
                    } else if( percent >=50 && percent < 74) {
                        lv_img_set_src( batteryicon, LV_SYMBOL_BATTERY_3 );
                    } else if( percent >=35 && percent < 49) {
                        lv_img_set_src( batteryicon, LV_SYMBOL_BATTERY_2 );
                    } else if( percent >=15 && percent < 34) {
                        lv_img_set_src( batteryicon, LV_SYMBOL_BATTERY_1 );
                        lv_obj_set_style_local_image_recolor( batteryicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW );
                    } else if( percent >=0 && percent < 14) {
                        lv_img_set_src( batteryicon, LV_SYMBOL_BATTERY_EMPTY );
                        lv_obj_set_style_local_image_recolor( batteryicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED );
                    }
                }
                break;
            }
    }
    main_tile_align_widgets();
    return( true );
}

static bool mainbar_blectl_event_cb( EventBits_t event, void *arg ) {
    static bool blectl_state = false;
    
    switch( event ) {
        case BLECTL_ON:
            blectl_state = true;
            lv_obj_set_hidden( bluetoothicon, false );
            lv_obj_set_style_local_image_recolor( bluetoothicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY );
            break;
        case BLECTL_OFF:
            blectl_state = false;
            lv_obj_set_hidden( bluetoothicon, true );
            lv_obj_set_style_local_image_recolor( bluetoothicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED );
            break;
        case BLECTL_CONNECT:
            if( blectl_state )
                lv_obj_set_style_local_image_recolor( bluetoothicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE );
            break;
        case BLECTL_DISCONNECT:
            if( blectl_state )
                lv_obj_set_style_local_image_recolor( bluetoothicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY );
            break;
    }
    main_tile_align_widgets();
    return( true );
}

static bool mainbar_wifictl_event_cb( EventBits_t event, void *arg ) {
    static bool wifictl_state = false;
    
    switch( event ) {
        case WIFICTL_ON:
            wifictl_state = true;
            lv_obj_set_hidden( wifiicon, false );
            lv_obj_set_style_local_image_recolor( wifiicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY );
            break;
        case WIFICTL_OFF:
            wifictl_state = false;
            lv_obj_set_hidden( wifiicon, true );
            lv_obj_set_style_local_image_recolor( wifiicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED );
            break;
        case WIFICTL_CONNECT:
            if( wifictl_state )
                lv_obj_set_style_local_image_recolor( wifiicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE );
            break;
        case WIFICTL_DISCONNECT:
            if( wifictl_state )
                lv_obj_set_style_local_image_recolor( wifiicon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY );
            break;
    }
    main_tile_align_widgets();
    return( true );    
}

static bool main_tile_style_event_cb( EventBits_t event, void *arg ){
    switch( event ) {
        case STYLE_CHANGE:     style = ws_get_mainbar_style();

                                lv_style_copy( &timestyle, style);
                                lv_style_set_text_font( &timestyle, LV_STATE_DEFAULT, time_font );

                                lv_style_copy( &datestyle, style);
                                lv_style_set_text_font( &datestyle, LV_STATE_DEFAULT, date_font );

                                lv_style_copy( &infostyle, style);
                                lv_style_set_text_font( &infostyle, LV_STATE_DEFAULT, info_font );

                                lv_style_copy( &tempstyle, style);
                                lv_style_set_text_font( &tempstyle, LV_STATE_DEFAULT, temp_font );

                                lv_style_copy( &iconstyle, style);
                                lv_style_set_text_font( &iconstyle, LV_STATE_DEFAULT, icon_font );
                                break;
    }
    return( true );
}

static bool main_tile_sensor_event_cb( EventBits_t event, void *arg ) {
    static float temp = 0.0f;
    static float humidity = 0.0f;

    switch( event ) {
        case SENSOR_TEMPERATURE:        temp = *(float *)arg;
                                        break;
        case SENSOR_RELHUMIDITY:        humidity = *(float*)arg;
                                        break;
    }

    char sensor_str[128] = "";
    snprintf( sensor_str, sizeof( sensor_str ), "temp/humidity: %0.1f°C/%.0f%%", temp, humidity );
    lv_label_set_text( templabel, sensor_str );
    lv_obj_align( templabel, infolabel, LV_ALIGN_OUT_TOP_MID, 0, 0 );

    return( true );
}

static bool main_tile_button_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case BUTTON_RIGHT:  mainbar_jump_to_tilenumber( app_tile_get_tile_num(), LV_ANIM_OFF );
                            mainbar_clear_history();
                            break;
    }
    return( true );
}


static bool main_tile_time_update_ebent_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case TIME_SYNC_UPDATE:
            main_tile_update_time( true );
            break;
    }
    return( true );
}

static bool main_tile_powermgm_event_cb( EventBits_t event, void *arg ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        return( true );
    }

    switch( event ) {
        case POWERMGM_WAKEUP:
            main_tile_update_time( true );
            break;
    }
    return( true );
}

lv_obj_t *main_tile_register_widget( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        while( true );
    }
    /**
     * search for a free widget and register it
     */
    for( int widget = 0 ; widget < MAX_WIDGET_NUM ; widget++ ) {
        if ( widget_entry[ widget ].active == false ) {
            widget_entry[ widget ].active = true;
            lv_obj_set_hidden( widget_entry[ widget ].icon_cont, false );
            main_tile_align_widgets();
            return( widget_entry[ widget ].icon_cont );
        }
    }
    log_e("no more space for a widget");
    return( NULL );
}

icon_t *main_tile_get_free_widget_icon( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        while( true );
    }
    /**
     * search for a free widget and return the number
     */
    for( int widget = 0 ; widget < MAX_WIDGET_NUM ; widget++ ) {
        if ( widget_entry[ widget ].active == false ) {
            lv_obj_set_hidden( widget_entry[ widget ].icon_cont, false );
            return( &widget_entry[ widget ] );
        }
    }
    log_e("no more space for a widget");
    return( NULL );
}

void main_tile_align_widgets( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        return;
    }

    int active_widgets = 0;
    lv_coord_t xpos = 0;
    /**
     * count the number of active widgets
     */
    for ( int widget = 0 ; widget < MAX_WIDGET_NUM ; widget++ ) {
        if ( widget_entry[ widget ].active )
        active_widgets++;
    }
    lv_obj_align( batteryicon, info_cont, LV_ALIGN_CENTER, 0, 0 );
    lv_obj_align( wifiicon, batteryicon, LV_ALIGN_OUT_LEFT_MID, -THEME_PADDING, 0 );
    lv_obj_align( bluetoothicon, wifiicon, LV_ALIGN_OUT_LEFT_MID, -THEME_PADDING, 0 );
    lv_obj_align( batterylabel, batteryicon, LV_ALIGN_OUT_RIGHT_MID, THEME_PADDING, 0 );
    /**
     * if we habe zero active widget, realign with no widgets
     */
    if ( active_widgets == 0 ) {
        lv_obj_align( clock_cont, main_cont, LV_ALIGN_CENTER, 0, 0 );
        return;
    };
    /**
     * set clock container to the top
     */
    lv_obj_align( clock_cont, main_cont, LV_ALIGN_IN_TOP_MID, 0, 0 );
    /**
     * align the widgets
     */
    xpos = 0 - ( ( WIDGET_X_SIZE * active_widgets ) + ( ( active_widgets - 1 ) * WIDGET_X_CLEARENCE ) ) / 2;
    active_widgets = 0;
    for ( int widget = 0 ; widget < MAX_WIDGET_NUM ; widget++ ) {
        if ( widget_entry[ widget ].active ) {
            lv_obj_align( widget_entry[ widget ].icon_cont , main_cont, LV_ALIGN_IN_BOTTOM_MID, xpos + ( WIDGET_X_SIZE * active_widgets ) + ( active_widgets * WIDGET_X_CLEARENCE ) + 32 , - ( ( lv_disp_get_ver_res( NULL ) / 4 ) -32 ) );
            active_widgets++;
        }
    }
}

uint32_t main_tile_get_tile_num( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        while( true );
    }

    return( main_tile_num );
}

void main_tile_update_time( bool force ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        return;
    }

    time_t now;
    static time_t last = 0;
    struct tm  info, last_info;
    char time_str[64]="";
    char info_str[64]="";
    /*
     * copy current time into now and convert it local time info
     */
    time( &now );
    localtime_r( &now, &info );
    /*
     * convert last time_t into tm from
     * last check if last equal zero (first run condition)
     */
    if ( last != 0 ) {
        localtime_r( &last, &last_info );
    }
    /*
     * Time:
     * only update while time changes or force is set
     * Display has a minute resolution
     */
    if ( last == 0 || info.tm_min != last_info.tm_min || info.tm_hour != last_info.tm_hour || force ) {
        timesync_get_current_timestring( time_str, sizeof(time_str) );
        log_d("renew time: %s", time_str );
        lv_label_set_text( timelabel, time_str );
        lv_obj_align( timelabel, clock_cont, LV_ALIGN_CENTER, 0, 0 );
        /*
         * Date:
         * only update while date changes
         */
        if ( last == 0 || info.tm_yday != last_info.tm_yday ) {
            strftime( time_str, sizeof(time_str), "%a %d.%b %Y", &info );
            log_d("renew date: %s", time_str );
            lv_label_set_text( datelabel, time_str );
            lv_obj_align( datelabel, clock_cont, LV_ALIGN_IN_BOTTOM_MID, 0, 0 );
        }

        snprintf( info_str, sizeof( info_str ),"battery: %d%%", pmu_get_battery_percent() );
        lv_label_set_text( infolabel, info_str );
        lv_obj_align( infolabel, datelabel, LV_ALIGN_OUT_TOP_MID, 0, 0 );
        /*
         * Save for next loop
         */
        last = now;
    }
}

static void main_tile_update_task( lv_task_t * task ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        return;
    }

    main_tile_update_time( false );
}
