#ifndef _ASTUTE_TCP_H
    #define _ASTUTE_TCP_H
#include "../callback.h"


    /**
     * connection state
     */
    #define TCPCTL_CONNECT               _BV(0)         /** @brief event mask for tcpctl connect to an client */
    #define TCPCTL_DISCONNECT            _BV(1)         /** @brief event mask for tcpctl disconnect */
    #define TCPCTL_NEW_IMAGE             _BV(2)         
    #define TCPCTL_DEBUG_MESSAGE         _BV(3)         


void tcp_setup( void );


bool tcp_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id );

#endif