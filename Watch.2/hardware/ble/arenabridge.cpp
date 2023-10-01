#include "arenabridge.h"

#include "hardware/blectl.h"
#include "TWatch_hal.h"

#include "astute-compression/AstuteDecode.h"

static bool arena_send_event_cb( EventBits_t event, void *arg );
void *_imgPtr;
#ifdef NATIVE_64BIT
#else
    #if defined( M5PAPER )
    #elif defined( M5CORE2 )
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
    #elif defined( LILYGO_WATCH_2021 )
    #elif defined( WT32_SC01 )
    #else
        #warning "no hardware driver for blearenabridge"
    #endif

    #include <Arduino.h>
    #include "NimBLEDescriptor.h"


    NimBLECharacteristic *pArenabridgeTXCharacteristic = NULL;         /** @brief TX Characteristic */
    NimBLECharacteristic *pArenabridgeRXCharacteristic = NULL;         /** @brief RX Characteristic */
    lv_img_dsc_t img_data;

#define NEW_FILE (unsigned char)1
#define CHUNK (unsigned char)2
#define LAST_CHUNK (unsigned char)4

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


void *imageBuffer;
int imageBufferSize;
int imageOffset;
class ArenaCharacteristicCallbacks: public NimBLECharacteristicCallbacks {

    int imageWidh;
    int imageHeight;
public:
    ArenaCharacteristicCallbacks()
    :  NimBLECharacteristicCallbacks() {
        imageBuffer = NULL;
        imageBufferSize = 0;
        imageOffset = 0;
    }

    void onRead(NimBLECharacteristic* pCharacteristic){
        size_t msgLen = pCharacteristic->getValue().length();

        std::string msg = pCharacteristic->getValue();
        //arena_send_event_cb(BLECTL_DEBUG_MESSAGE, (void*)msg.c_str());
    
        log_i("onRead: Have new message with size: %d", msgLen);

    };
    
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        int messageSize =  pCharacteristic->getValue().length();
        if (messageSize <= 0) {
            return;
        }
        int offset = 0;

        unsigned char type;
        int bufSize = sizeof(unsigned char);
        memcpy(&(type), (void *)pCharacteristic->getValue().data(), bufSize);
        offset += bufSize;
        //printf("%d Have new chunk!!!!! \n",  millis());

        switch (type)
        {
        case NEW_FILE: {
            log_i("%d==start_upload_file", millis());

            if (imageBuffer != NULL) {
                //free(imageBuffer);
                imageBufferSize = 0;
                imageBuffer = NULL;
                imageOffset = 0;
            }

            int bufSize = sizeof(int);

            memcpy(&(imageBufferSize), pCharacteristic->getValue().data() + offset, bufSize);
            offset += bufSize;

            memcpy(&(imageWidh), pCharacteristic->getValue().data() + offset, bufSize);
            offset += bufSize;
            img_data.header.w = imageWidh;

            memcpy(&(imageHeight), pCharacteristic->getValue().data() + offset, bufSize);
            offset += bufSize;
            img_data.header.h = imageHeight;

            imageBuffer = malloc(imageBufferSize);
            memcpy(imageBuffer + imageOffset, pCharacteristic->getValue().data() + offset, messageSize - offset);
            imageOffset += messageSize - offset;
            log_i("Image width: %d, height %d, size %d", imageWidh, imageHeight, imageBufferSize);

            break;
        }
        case CHUNK: {
            if (imageBuffer == NULL) {
                return;
            }
            memcpy(imageBuffer + imageOffset, pCharacteristic->getValue().data() + offset, messageSize - offset);
            imageOffset += messageSize - offset;
            
        }
            break;
        case LAST_CHUNK: {
            log_i("%d ==last_chunk", millis());
            if (imageBuffer == NULL) {
                return;
            }
            memcpy(imageBuffer + imageOffset, pCharacteristic->getValue().data() + offset, messageSize - offset);
            imageOffset += messageSize - offset;


            AWDecoder decoder = {};

            log_i("==aw_decoder_init");
            aw_decoder_init(&decoder, SLocal::Receiver, &local);
            log_i("==aw_decoder_finished");

            int len = imageBufferSize;
            while (len)
            {
                size_t left = 0;
                void* tail = aw_get_tail(&decoder, left);
                //assert(left);

                size_t size = len < left ? len : left;
                memcpy(tail, imageBuffer, size);
                decoder.filled += size;

                int decode_res = aw_decoder_chunk(&decoder);
                assert(0 == decode_res);
                len -= size;
                imageBuffer = (char*)imageBuffer + size;
                //log_i("decoder.filled %d left %d size %d", decoder.filled, left, size);
                log_i("----res.size %d", local.res.size());
            }
            
            log_i("==aw_decoder_fini_start");
            aw_decoder_fini(&decoder, false);
            log_i("==aw_decoder_fini_finish");


            log_i("!!!!res.size %d", local.res.size());

            img_data.data = (uint8_t *)local.res.data();
            img_data.data_size = 240*240*2;
            img_data.header.cf = LV_IMG_CF_TRUE_COLOR;
            img_data.header.always_zero = 0;
            
            /*log_i("==start_draw");
            arena_send_event_cb(BLECTL_NEW_IMAGE, (void*) &img_data);
            log_i("==end_draw");

*/

            // move to render!!!!
            log_i("Try show image");
            lv_img_set_src((lv_obj_t*)_imgPtr, &img_data);
            //free(imageBuffer);


            log_i("==start_send_notify");
            pArenabridgeTXCharacteristic->setValue((void*)"ratatata");
            pArenabridgeTXCharacteristic->notify();
            log_i("==end_send_notify");
        }
            break;
        default:
            log_e("unsupported chunk format");
            break;
        }
    
        //log_i("onWrite: Have new message with size: %d total: %d count: %d type %d", messageSize, sended_data, count, type);
   
    }
};

static ArenaCharacteristicCallbacks arenabridge_callb;
#endif

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