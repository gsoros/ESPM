#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "ble_constants.h"
#include "task.h"

class BLE : public BLEServerCallbacks, public Task {
   public:
    BLEServer *server;
    BLECharacteristic *cpmChar;  // cycling power measurement char
    BLECharacteristic *blChar;   // battery level char
    uint8_t batteryLevel = 0;
    uint8_t oldBatteryLevel = 0;
    bool connected = false;
    bool oldConnected = false;
    unsigned long lastNotificationSent = 0;

    short power = 0;
    unsigned short revolutions = 0;
    unsigned short timestamp = 0;
    const unsigned short flags = 0x20;  // TODO

    unsigned char bufMeasurent[8];
    unsigned char bufSensorLocation[1];
    unsigned char bufControlPoint[1];
    unsigned char bufFeature[4];

    void setup(const char *deviceName);
    void loop(const ulong t);

    void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc);
    void onDisconnect(BLEServer *pServer);

    //bool onConfirmPIN(uint32_t pin);
    //uint32_t onPassKeyRequest();
    //void onAuthenticationComplete(ble_gap_conn_desc *desc);
};

#endif