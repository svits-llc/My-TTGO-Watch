#include "arenabridge.h"

#include "hardware/blectl.h"
#include "TWatch_hal.h"

#include "../../lib/astute-compression/AstuteDecode.h"
#include "../../lib/astute-compression/aw_profile.hpp"

static bool arena_send_event_cb( EventBits_t event, void *arg );
void *_imgPtr;

    #include <Arduino.h>
    #include "NimBLEDescriptor.h"

#include <iostream>
#include <sstream>

    NimBLECharacteristic *pArenabridgeTXCharacteristic = NULL;         /** @brief TX Characteristic */
    NimBLECharacteristic *pArenabridgeRXCharacteristic = NULL;         /** @brief RX Characteristic */
    lv_img_dsc_t img_data;

#define NEW_FILE (unsigned char)1
#define CHUNK (unsigned char)2
#define LAST_CHUNK (unsigned char)4
#define SEND_DEBUG_INFO (unsigned char)8



#define RECIVE_UPDATE (unsigned char)1
#define START_RESIVE_DEBUG_HEADER (unsigned char)2
#define RESIVE_DEBUG_DH_CHUNK (unsigned char)3
#define END_RESIVE_DEBUG_HEADER (unsigned char)4

#define START_RESIVE_DEBUG_DATA (unsigned char)5
#define RESIVE_DEBUG_DATA_CHUNL (unsigned char)6
#define END_RESIVE_DEBUG_DATA (unsigned char)7

struct SLocal {
    //SLocal() {
    //    res.resize(240*240);
    //    log_i("memory occupy succeed!");
    //}
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
};

class ArenaCharacteristicCallbacks: public NimBLECharacteristicCallbacks {

    int imageWidh;
    int imageHeight;
public:
    ArenaCharacteristicCallbacks()
    :  NimBLECharacteristicCallbacks() {
        //_paint_qr();
    }


    
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        int messageSize =  pCharacteristic->getValue().length();

        if (messageSize <= 0) {
            return;
        }
        int offset = 0;

        unsigned char type;

        memcpy(&type, pCharacteristic->getValue().data(), sizeof(type));
        offset += sizeof(type);


        switch (type)
        {
        case NEW_FILE: {
            AW_PROFILE_SCOPE("NEW_FILE");
            int fileSize = 0;
            int bufSize = sizeof(int);
            memcpy(&fileSize, pCharacteristic->getValue().data() + offset, bufSize);
            offset += bufSize;

            memcpy(&imageWidh, pCharacteristic->getValue().data() + offset, bufSize);
            offset += bufSize;
            img_data.header.w = imageWidh;

            memcpy(&imageHeight, pCharacteristic->getValue().data() + offset, bufSize);
            offset += bufSize;
            img_data.header.h = imageHeight;


            aw_decoder_init(&_decoder, SLocal::Receiver, &_local);
            _decode((uint8_t *)(pCharacteristic->getValue().data() + offset), messageSize - offset);

            break;
        }
        case CHUNK: {
            AW_PROFILE_SCOPE("CHUNK");
            _decode((uint8_t *)(pCharacteristic->getValue().data() + offset), messageSize - offset);
        }
            break;
        case LAST_CHUNK: {
            AW_PROFILE_SCOPE("LAST_CHUNK");

            _decode((uint8_t *)(pCharacteristic->getValue().data() + offset), messageSize - offset);

            aw_decoder_fini(&_decoder, false);

            _paint();
            unsigned char header[1];
            header[0] = RECIVE_UPDATE;
            pArenabridgeTXCharacteristic->setValue(header, 1);
            pArenabridgeTXCharacteristic->notify();
        }
            break;
        case SEND_DEBUG_INFO: {
            log_i("Waiting debug information");
            std::ostringstream headers;
            std::ostringstream file;

            CProfileFunction::GetProfiler().PrintHeader(headers);
            CProfileFunction::GetProfiler().BlobToText(file);
            printf("Headers: \n %s \n", headers.str().c_str());
            printf("Data: \n %s \n", file.str().c_str());

            _sendDebugHeader(headers.str());
            _sendDebugData(file.str());            
        }
        break;
        default:
            log_e("unsupported chunk format");
            break;
        }   
    }
    private:

    void _decode(uint8_t *buffer, size_t len) {
        AW_PROFILE_SCOPE("decode");
        int decode_res = aw_data_chunk(&_decoder, len, buffer);
        assert(decode_res == 0);
        /*while (len)
            {
                log_i("decoder cap is %d", _decoder.header.cap);
                size_t left = 0;
                void* tail = aw_get_tail(&_decoder, &left);
                //assert(left);

                size_t size = len < left ? len : left;
                memcpy(tail, buffer, size);
                _decoder.filled += size;

                //int decode_res = aw_decoder_chunk(&_decoder);
                log_i("decode_res is %d", decode_res);
                assert(0 == decode_res);
                len -= size;
                buffer = (uint8_t*)buffer + size;
            }
            */
    }

    void _paint() {
            AW_PROFILE_SCOPE("paint");
            img_data.data = (uint8_t *)_local.res.data();
            img_data.data_size = img_data.header.w * img_data.header.h * 2;
            img_data.header.cf = LV_IMG_CF_TRUE_COLOR;
            img_data.header.always_zero = 0;
            
            // move to render!!!!
            lv_img_set_src((lv_obj_t*)_imgPtr, &img_data);
    }

    void _paint_qr() {
            AW_PROFILE_SCOPE("draw_qr");
            aw_decoder_init(&_decoder, SLocal::Receiver, &_local);
            int qr_res = aw_draw_qr(&_decoder);
            assert(qr_res == 0);

            img_data.data = (uint8_t *)_local.res.data();
            img_data.data_size = img_data.header.w * img_data.header.h * 2;
            img_data.header.cf = LV_IMG_CF_TRUE_COLOR;
            img_data.header.always_zero = 0;
            
            // move to render!!!!
            lv_img_set_src((lv_obj_t*)_imgPtr, &img_data);
    }

    void _sendDebugHeader(std::string str) {
        int chunk = 243;
        int bufSize = str.size();
        for (int i = 0; i < bufSize; i += chunk) {
            int remainingSize = bufSize - i;
            int chunkLength = remainingSize < chunk ? remainingSize : chunk;
            
            uint8_t * chunkData = (uint8_t *)malloc(chunkLength + 1);

            chunkData[0] =  i == 0 ? START_RESIVE_DEBUG_HEADER : (remainingSize < chunk ? END_RESIVE_DEBUG_HEADER : RESIVE_DEBUG_DH_CHUNK);
            memcpy(chunkData + 1, str.substr(i, chunkLength).c_str(), chunkLength);

            pArenabridgeTXCharacteristic->setValue(chunkData, chunkLength + 1);
            pArenabridgeTXCharacteristic->notify();
        }
    }

    void _sendDebugData(std::string str) {
        int chunk = 243;
        int bufSize = str.size();
        for (int i = 0; i < bufSize; i += chunk) {
            int remainingSize = bufSize - i;
            int chunkLength = remainingSize < chunk ? remainingSize : chunk;
            
            uint8_t * chunkData = (uint8_t *)malloc(chunkLength + 1);

            chunkData[0] =  i == 0 ? START_RESIVE_DEBUG_DATA : (remainingSize < chunk ? END_RESIVE_DEBUG_DATA : RESIVE_DEBUG_DATA_CHUNL);
            memcpy(chunkData + 1, str.substr(i, chunkLength).c_str(), chunkLength);

            pArenabridgeTXCharacteristic->setValue(chunkData, chunkLength + 1);
            pArenabridgeTXCharacteristic->notify();
        }
    }
    SLocal _local;
    AWDecoder _decoder = {};

};

static ArenaCharacteristicCallbacks arenabridge_callb;

void arenabridge_setup( void *imagePtr ) {
    _imgPtr = imagePtr;

    #ifdef NATIVE_64BIT
    #else
        /**
         * Create the BLE Service
         */
        NimBLEServer *pServer = blectl_get_ble_server();
        NimBLEAdvertising *pAdvertising = blectl_get_ble_advertising();
        NimBLEService *pArenabridgeService = pServer->createService( NimBLEUUID( ARENABRIDGE_SERVICE_UUID ) );
        /**
         * Create Arenabridge TX/RX Characteristic
         */
        pArenabridgeTXCharacteristic = pArenabridgeService->createCharacteristic( NimBLEUUID( ARENABRIDGE_CHARACTERISTIC_UUID_TX ), NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE );
        pArenabridgeTXCharacteristic->addDescriptor( new NimBLE2904() );
        pArenabridgeRXCharacteristic = pArenabridgeService->createCharacteristic( NimBLEUUID( ARENABRIDGE_CHARACTERISTIC_UUID_RX ), NIMBLE_PROPERTY::WRITE  | NIMBLE_PROPERTY::WRITE_NR);
        pArenabridgeRXCharacteristic->setCallbacks( &arenabridge_callb );
        pArenabridgeService->start();
        /**
         * add service to advertising
         */
        pAdvertising->addServiceUUID( pArenabridgeService->getUUID() );

       // arena_send_event_cb(BLECTL_DEBUG_MESSAGE, (void*)"all inited");
    #endif
}

/*bool arenabridge_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id ) {
    if ( arenatl_callback == NULL ) {
        arenatl_callback = callback_init( "arenabridge" );
        ASSERT( arenatl_callback, "arenabridge callback alloc failed" );
    }    
    return( callback_register( arenatl_callback, event, callback_func, id ) );
}

static bool arena_send_event_cb( EventBits_t event, void *arg ) {
    return( callback_send( arenatl_callback, event, arg ) );
}*/