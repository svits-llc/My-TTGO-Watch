#include "arenabridge.h"

#include "hardware/blectl.h"
#include "hardware/callback.h"
#include "hardware/pmu.h"
#include "hardware/powermgm.h"
#include "utils/alloc.h"


callback_t *arenatl_callback = NULL;         /** @brief tcpctl callback structure */

static bool arena_send_event_cb( EventBits_t event, void *arg );

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
    #include "gui/widget_factory.h"


    NimBLECharacteristic *pArenabridgeTXCharacteristic = NULL;         /** @brief TX Characteristic */
    NimBLECharacteristic *pArenabridgeRXCharacteristic = NULL;         /** @brief RX Characteristic */
    lv_img_dsc_t img_data;

    
#endif


#define NEW_FILE (unsigned char)1
#define CHUNK (unsigned char)2
#define LAST_CHUNK (unsigned char)4

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

        switch (type)
        {
        case NEW_FILE: {
            log_i("!!!!!!start recive new file");

            if (imageBuffer != NULL) {
                free(imageBuffer);
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
        case LAST_CHUNK:
            if (imageBuffer == NULL) {
                return;
            }
            memcpy(imageBuffer + imageOffset, pCharacteristic->getValue().data() + offset, messageSize - offset);
            imageOffset += messageSize - offset;

            img_data.data = (uint8_t *)imageBuffer;
            img_data.data_size = imageBufferSize;
            img_data.header.cf = LV_IMG_CF_TRUE_COLOR;
            img_data.header.always_zero = 0;
            
            log_i("!!!!!LAST CHUNK");
            arena_send_event_cb(BLECTL_NEW_IMAGE, (void*) &img_data);
            // move to render!!!!
            break;
        default:
            log_e("unsupported chunk format");
            break;
        }
    
        //log_i("onWrite: Have new message with size: %d total: %d count: %d type %d", messageSize, sended_data, count, type);
    }
};

static ArenaCharacteristicCallbacks arenabridge_callb;

void arenabridge_setup( void ) {


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
        pArenabridgeTXCharacteristic = pArenabridgeService->createCharacteristic( NimBLEUUID( ARENABRIDGE_CHARACTERISTIC_UUID_TX ), NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ );
        pArenabridgeTXCharacteristic->addDescriptor( new NimBLE2904() );
        pArenabridgeRXCharacteristic = pArenabridgeService->createCharacteristic( NimBLEUUID( ARENABRIDGE_CHARACTERISTIC_UUID_RX ), NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ );
        pArenabridgeRXCharacteristic->setCallbacks( &arenabridge_callb );
        pArenabridgeService->start();
        /**
         * add service to advertising
         */
        pAdvertising->addServiceUUID( pArenabridgeService->getUUID() );

        arena_send_event_cb(BLECTL_DEBUG_MESSAGE, (void*)"all inited");
    #endif
}

bool arenabridge_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id ) {
    if ( arenatl_callback == NULL ) {
        arenatl_callback = callback_init( "arenabridge" );
        ASSERT( arenatl_callback, "arenabridge callback alloc failed" );
    }    
    return( callback_register( arenatl_callback, event, callback_func, id ) );
}

static bool arena_send_event_cb( EventBits_t event, void *arg ) {
    return( callback_send( arenatl_callback, event, arg ) );
}