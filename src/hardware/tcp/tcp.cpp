#include "tcp.h"

#include "utils/alloc.h"
#include "gui/widget_factory.h"

#ifdef NATIVE_64BIT
    #include "../../utils/logging.h"
    #include "tcp_server/tcp_server.h"

    TcpServer *server;
#endif


callback_t *tcpctl_callback = NULL;         /** @brief tcpctl callback structure */

static bool tcpctl_send_event_cb( EventBits_t event, void *arg );

static void client_connected_cb();
static void new_image_cb(lv_img_dsc_t *img);

void tcp_setup( void ) {
        log_i("setup tcp server");
#ifdef NATIVE_64BIT
    server = new TcpServer();

    server->startListening(3445, client_connected_cb, new_image_cb);
            log_i("!!!!!!");
#endif
}

bool tcp_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id ) {
        if ( tcpctl_callback == NULL ) {
        tcpctl_callback = callback_init( "tcpctl" );
        ASSERT( tcpctl_callback, "tcpctl callback alloc failed" );
    }    
    return( callback_register( tcpctl_callback, event, callback_func, id ) );
}

static bool tcpctl_send_event_cb( EventBits_t event, void *arg ) {
    return( callback_send( tcpctl_callback, event, arg ) );
}

static void client_connected_cb() {
    log_i("have new connection");
    tcpctl_send_event_cb(TCPCTL_CONNECT, NULL);
}

static void new_image_cb(lv_img_dsc_t *img) {
        log_i("have new image !!!!!!!");
        tcpctl_send_event_cb(TCPCTL_NEW_IMAGE, (void*)img);
}
