#ifndef _ARENABRIDGE_H
    #define _ARENABRIDGE_H
#include "../callback.h"

    #define ARENABRIDGE_SERVICE_UUID                           "6E400004-B5A3-F393-E0A9-E50E24DCCA9E"     /** @brief UART service UUID */
    #define ARENABRIDGE_CHARACTERISTIC_UUID_RX                 "6E400005-B5A3-F393-E0A9-E50E24DCCA9E"
    #define ARENABRIDGE_CHARACTERISTIC_UUID_TX                 "6E400006-B5A3-F393-E0A9-E50E24DCCA9E"

    #define BLECTL_NEW_IMAGE             _BV(2)         
    #define BLECTL_DEBUG_MESSAGE         _BV(3)         


    void arenabridge_setup( void );
    bool arenabridge_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id );

#endif