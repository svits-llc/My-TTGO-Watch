#ifndef _BLECTL_H
    #define _BLECTL_H

    #include "NimBLEDevice.h"

    void blectl_setup( void *imagePtr);


#ifndef NATIVE_64BIT
    /**
     * @brief get the raw BLE Server
     */
    NimBLEServer *blectl_get_ble_server( void );
    /**
     * @brief get the raw BLE Advertising
     */
    NimBLEAdvertising *blectl_get_ble_advertising( void );
#endif

#endif // _BLECTL_H