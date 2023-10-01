#ifdef NATIVE_64BIT

#include "tcp_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring> 
#include <memory>
#include <vector>
#include <functional>

#include <iostream>
#include "../../astute.arena/AstuteDecode.h"

lv_img_dsc_t img_data;


void serverHandler(int serverSocket, TcpServer *server) {
        while (true) {

            sockaddr_in clientAddress{};
            socklen_t clientAddressLength = sizeof(clientAddress);
            int clientSocket = accept(serverSocket, (struct sockaddr*) &clientAddress, &clientAddressLength);

            if (clientSocket == -1) {
                std::cerr << "failed accept client" << std::endl;
                close(serverSocket);
                return;
            }

            server->haveNewConnection(clientSocket);
    }
}

struct SLocal {
    static void Receiver(void* cookie, size_t offset, size_t count, uint16_t* pix)
    {
        SLocal* pSelf = reinterpret_cast<SLocal*>(cookie);
        size_t size = offset + count;
        if (size > pSelf->res.size())
        {
            pSelf->res.resize(size);
        }
        memcpy(&pSelf->res[offset], pix, count * sizeof(uint16_t));
    }
    std::vector<uint16_t> res;
} local;


TcpServer::TcpServer()
{
    serverSocket = -1;
    clientSocket = -1;
}

TcpServer::~TcpServer()
{
    if (serverSocket > 0) {
        close(serverSocket);
    }

    if (clientSocket > 0) {
        close(clientSocket);
    }
}

void TcpServer::startListening(uint32_t port, client_connected_callb callb, new_image_callb image_callb)
{
    _connected_calb = callb;
    _new_image_calb = image_callb;
    
    //img_obj_ptr = img_obj;
    //memset(&img_data, 0, sizeof(lv_img_dsc_t));

    std::cout << "Start listening on port" << port << std::endl;
    int serverSocket, clientSocket;
    int serverPort = port;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "can't create socket" << std::endl;
        return;
    }

    std::cout << serverSocket << std::endl;

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Can't bind socket" << std::endl;
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 1) < 0) {
        std::cerr << "Error while client connected" << std::endl;
        close(serverSocket);
        return;
    }

    std::thread::id this_id = std::this_thread::get_id();
    std::cout << "startListening: thread_id: " << this_id << std::endl;

    listeningThread = new std::thread(serverHandler, serverSocket, this);

}

void TcpServer::haveNewConnection(int clientSocket)
{
    if (_connected_calb != NULL) {
        _connected_calb();
        std::thread::id this_id = std::this_thread::get_id();
        std::cout << "haveNewConnection: thread_id: " << this_id << std::endl;
    }

    while(1) {

        int offset = 0;
        int messageSize = 0;
        char headBuffer[12];
        std::cerr << "start recv" << std::endl;

        ssize_t bytesReceived = recv(clientSocket, headBuffer, sizeof(headBuffer), MSG_WAITALL);

        if (bytesReceived == -1) {
            std::cerr << "Error resived" << bytesReceived << std::endl;
            close(clientSocket);
            return;
        } else if (bytesReceived == 0) {
            std::cout << "Closed from client side" << std::endl;
            close(clientSocket);
            return;
        } else {
            std::cout << "!!!!!!!!!!!!!!!!!!!!!!received " << bytesReceived << std::endl;

           // img_data = (lv_img_dsc_t*)malloc(sizeof(lv_img_dsc_t));
            int bufSize = sizeof(int);

            memcpy(&(messageSize), headBuffer + offset, bufSize);
            offset += bufSize;

            int imageWidh, imageHeight = 0;

            memcpy(&(imageWidh), headBuffer + offset, bufSize);
            offset += bufSize;
            img_data.header.w = htonl(imageWidh);

            memcpy(&(imageHeight), headBuffer + offset, bufSize);
            offset += bufSize;
            img_data.header.h = htonl(imageHeight);

            std::cout << "message size " << messageSize << " image width " <<  imageWidh << " image heigth " << imageHeight << std::endl;
            int imageBufferSize = messageSize - (int)sizeof(headBuffer);

            void *imageBuffer = malloc(imageBufferSize);
            std::cout << "image buffer size " << imageBufferSize << std::endl;

            ssize_t bbytesReceived = recv(clientSocket, imageBuffer, imageBufferSize, MSG_WAITALL);

            std::cout << "received " << bbytesReceived << std::endl;

            AWDecoder decoder = {};

            aw_decoder_init(&decoder, SLocal::Receiver, &local);
            int len = imageBufferSize;

/*
            while (len)
            {
                size_t left = AW_BUFF_SIZE - decoder.filled;
                size_t size = len < left ? len : left;

                memcpy(decoder.buff + decoder.filled, imageBuffer, size);
                decoder.filled += size;

                aw_decoder_chunk(&decoder);
                len -= size;
                imageBuffer = (char*)imageBuffer + size;
            }

            aw_decoder_fini(&decoder);

*/
            for (int i = 0; i < 3*50; i+=3) {
                std::cout << int(*(uint8_t *)(imageBuffer + i)) << int(*(uint8_t *)(imageBuffer + i + 1)) << int(*(uint8_t *)(imageBuffer + i + 2))<< std::endl;
            } 
            img_data.data = (uint8_t *)imageBuffer;//(uint8_t *)local.res.data();
            img_data.data_size = 240*240*3;
            img_data.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
            img_data.header.always_zero = 0;

            _new_image_calb(&img_data);

           // free(imageBuffer);
           // free(img_data); !!! MEMORY LEAK

        }

    }
}

#endif