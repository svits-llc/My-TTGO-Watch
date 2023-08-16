#ifndef _ASTUTE_TCP_SERVER_APP_H
    #define _ASTUTE_TCP_SERVER_APP_H

#ifdef NATIVE_64BIT
    #include <cstdint>
    #include <mutex>
    #include <thread>
    #include "gui/widget_factory.h"


    typedef void ( * client_connected_callb ) ();
    typedef void ( * client_disconnected_callb ) ();
    typedef void ( * new_image_callb ) (lv_img_dsc_t *img);

    class TcpServer {
        public:
        TcpServer();
        virtual ~TcpServer();
    
        void startListening(uint32_t port, client_connected_callb callb, new_image_callb image_callb);
        void haveNewConnection(int clientSocket);

        private:

        std::thread *listeningThread;

        int serverSocket;
        std::mutex serverMutex;
    
        int clientSocket;
        std::mutex clientMutex;

       
       client_connected_callb _connected_calb;
       client_disconnected_callb _disconnected_calb;
       new_image_callb _new_image_calb;
    };
#endif

#endif // _ASTUTE_TCP_SERVER_APP_H