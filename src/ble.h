#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "ble_constants.h"
#include "task.h"

class BLE : public Task,
            public BLEServerCallbacks,
            public BLECharacteristicCallbacks {
   public:
    BLEServer *server;
    BLECharacteristic *cpmChar;   // cycling power measurement characteristic
    BLECharacteristic *cscmChar;  // cycling speed and cadence measurement characteristic
    BLECharacteristic *blChar;    // battery level characteristic
    uint8_t batteryLevel = 0;
    uint8_t prevBatteryLevel = 0;
    bool connected = false;
    bool prevConnected = false;
    unsigned long lastNotification = 0;

    short power = 0;
    uint16_t crankRevs = 0;
    uint16_t lastCrankEventTime = 0;
    const uint16_t powerFlags = 0x00;
    const uint8_t cadenceFlags = 0b00000010;  // Wheel rev data present = 0, Crank rev data present = 1

    unsigned char bufPower[4];
    unsigned char bufCadence[5];
    unsigned char bufSensorLocation[1];
    unsigned char bufControlPoint[1];
    unsigned char bufPowerFeature[4];
    unsigned char bufSpeedCadenceFeature[2];

    void setup(const char *deviceName);
    void loop();

    void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc);
    void onDisconnect(BLEServer *pServer);
    void startAdvertising();

    void onRead(BLECharacteristic *pCharacteristic);
    void onWrite(BLECharacteristic *pCharacteristic);
    void onNotify(BLECharacteristic *pCharacteristic);
    void onSubscribe(BLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue);

    //bool onConfirmPIN(uint32_t pin);
    //uint32_t onPassKeyRequest();
    //void onAuthenticationComplete(ble_gap_conn_desc *desc);
};

#endif