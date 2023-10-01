#include "config.h"
#include "blectl.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
    #include "utils/millis.h"

    static EventBits_t blectl_status;
#else
    #if defined( M5PAPER )
    #elif defined( M5CORE2 )
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
    #elif defined( LILYGO_WATCH_2021 )
    #elif defined( WT32_SC01 )
    #else
        #warning "no hardware driver for blectl"
    #endif

    #include <Arduino.h>
    #include "ble/arenabridge.h"

    #include "NimBLEDescriptor.h"

    EventGroupHandle_t blectl_status = NULL;
    portMUX_TYPE DRAM_ATTR blectlMux = portMUX_INITIALIZER_UNLOCKED;
#endif

#ifdef NATIVE_64BIT
#else
    NimBLEServer *pServer = NULL;                          
    NimBLEAdvertising *pAdvertising = NULL;

    class ServerCallbacks: public NimBLEServerCallbacks {
        void onConnect(NimBLEServer* pServer) {
            printf("Client connected");
            NimBLEDevice::startAdvertising();
        };

        void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
            /*pServer->updateConnParams(desc->conn_handle, blectl_config.minInterval, blectl_config.maxInterval, blectl_config.latency, blectl_config.timeout );
            blectl_set_event( BLECTL_AUTHWAIT );
            blectl_clear_event( BLECTL_DISCONNECT | BLECTL_CONNECT );
            powermgm_resume_from_ISR();
            log_d("BLE authwait");
            blectl_send_event_cb( BLECTL_AUTHWAIT, (void *)"authwait" );
            log_i("Client address: %s", NimBLEAddress(desc->peer_ota_addr).toString().c_str() );*/
            printf("Client address: ");
            printf(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
            NimBLEDevice::startAdvertising();

            /** We can use the connection handle here to ask for different connection parameters.
                *  Args: connection handle, min connection interval, max connection interval
                *  latency, supervision timeout.
                *  Units; Min/Max Intervals: 1.25 millisecond increments.
                *  Latency: number of intervals allowed to skip.
                *  Timeout: 10 millisecond increments, try for 5x interval time for best results.
            */
            pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
        };

        void onDisconnect(NimBLEServer* pServer) {
            Serial.println("Client disconnected - start advertising");
            NimBLEDevice::startAdvertising();

        };

        void onMTUChange( uint16_t MTU, ble_gap_conn_desc* desc ) {
            printf("MTU updated: %u for connection ID: %u\n", MTU, desc->conn_handle);
        };
        
        uint32_t onPassKeyRequest(){

            return 123456;
        };
        
        bool onConfirmPIN( uint32_t pass_key ) {
            printf("The passkey YES/NO number: ");
            printf("%d",pass_key);
            /** Return false if passkeys don't match. */
        return true;
        };

        void onAuthenticationComplete(ble_gap_conn_desc* desc){
            if(!desc->sec_state.encrypted) {
                NimBLEDevice::getServer()->disconnect(desc->conn_handle);
                Serial.println("Encrypt connection failed - disconnecting client");
                return;
            }
            Serial.println("Starting BLE work!");
        };
    };
#endif

void blectl_setup( void *imagePtr ) {
    #ifdef NATIVE_64BIT
    #else
        /**
         * allocate event group
         */
        blectl_status = xEventGroupCreate();
        //ASSERT( blectl_status, "Failed to allocate event group" );
        /**
         *  Create the BLE Device
         */

        NimBLEDevice::init( "Astute watch" );
        /*
         * set power level
         */
        NimBLEDevice::setPower( ESP_PWR_LVL_P9 );

        /*
         * Enable encryption and pairing options
         */
        //NimBLEDevice::setSecurityAuth( true, true, true );
        NimBLEDevice::setSecurityIOCap( BLE_HS_IO_DISPLAY_ONLY );
        NimBLEDevice::setMTU(247);
        /*
         * Create the BLE Server
         */
        pServer = NimBLEDevice::createServer();
        pServer->setCallbacks( new ServerCallbacks() );
        pAdvertising = NimBLEDevice::getAdvertising();
        pAdvertising->setAppearance( 0x00c0 );
        /**
         * add services
         */
        arenabridge_setup(imagePtr);
        //gadgetbridge_setup();
        //blebatctl_setup();
        //blestepctl_setup();

        /*
         * Start advertising
         */
        pAdvertising->start();
    #endif

  //  if( blectl_get_autoon() )
    //    blectl_on();

    //powermgm_register_cb_with_prio( POWERMGM_STANDBY, blectl_powermgm_event_cb, "powermgm blectl", CALL_CB_FIRST );
    //powermgm_register_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_WAKEUP, blectl_powermgm_event_cb, "powermgm blectl" );
}



NimBLEServer *blectl_get_ble_server( void ) {
    return pServer;
}
NimBLEAdvertising *blectl_get_ble_advertising( void ) {
    return pAdvertising;
}
