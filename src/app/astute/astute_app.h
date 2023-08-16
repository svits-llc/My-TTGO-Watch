#ifndef _ASTUTE_APP_H
    #define _ASTUTE_APP_H

    /**
     * @brief setup astute app
     * 
     */
    void astute_app_setup( void );
    /**
     * @brief get to astute app tile number
     * 
     * @return uint32_t tilenumber
     */
    uint32_t astute_app_get_app_main_tile_num( void );
    void enter_astute_app_event_cb( lv_obj_t * obj, lv_event_t event );
    void exit_astute_app_main_event_cb( lv_obj_t * obj, lv_event_t event );

#endif // _ASTUTE_APP_H