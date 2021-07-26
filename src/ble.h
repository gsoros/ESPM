#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "ble_constants.h"
#include "haspreferences.h"
#include "task.h"

class BLE : public Task,
            public HasPreferences,
            public BLEServerCallbacks,
            public BLECharacteristicCallbacks {
   public:
    BLEServer *server;
    BLEUUID cpsUUID;              // cycling power service uuid
    BLEService *cps;              // cycling power service
    BLECharacteristic *cpmChar;   // cycling power measurement characteristic
    BLEUUID cscsUUID;             // cycling speed and cadence service uuid
    BLEService *cscs;             // cycling speed and cadence service
    BLECharacteristic *cscmChar;  // cycling speed and cadence measurement characteristic
    BLEUUID blsUUID;              // battery level service uuid
    BLEService *bls;              // battery level service
    BLECharacteristic *blChar;    // battery level characteristic
    BLEUUID asUUID;               // api service uuid
    BLEService *as;               // api service
    BLECharacteristic *apiChar;   // api characteristic
    BLEAdvertising *advertising;

    uint8_t lastBatteryLevel = 0;
    unsigned long lastPowerNotification = 0;
    unsigned long lastCadenceNotification = 0;
    unsigned long lastBatteryNotification = 0;
    bool cadenceInCpm = true;
    bool cscServiceActive = false;

    uint16_t power = 0;
    uint16_t crankRevs = 0;
    uint16_t lastCrankEventTime = 0;                            // 1/1024s, rolls over
    const uint16_t powerFlags = 0b0000000000000000;             // Only instantaneous power present
    const uint16_t powerFlagsWithCadence = 0b0000000000100000;  // Crank rev data present
    const uint8_t cadenceFlags = 0b00000010;                    // Wheel rev data present = 0, Crank rev data present = 1

    unsigned char bufPower[8];    // [flags: 2][power: 2][revolutions: 2][last crank event: 2]
    unsigned char bufCadence[5];  // [flags: 1][revolutions: 2][last crank event: 2]
    unsigned char bufSensorLocation[1];
    unsigned char bufControlPoint[1];
    unsigned char bufPowerFeature[4];
    unsigned char bufSpeedCadenceFeature[2];

    void setup(const char *deviceName, Preferences *p);
    void loop();
    void startPowerService();
    void stopPowerService();
    void startCadenceService();
    void stopCadenceService();
    void startBatterySerice();
    void startApiSerice();
    void onCrankEvent(const ulong t, const uint16_t revolutions);
    void notifyPower(const ulong t);
    void notifyCadence(const ulong t);
    void notifyBattery(const ulong t);
    void setApiResponse(const char *response);

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

    void setCadenceInCpm(bool state);
    void setCscServiceActive(bool state);
    void loadSettings();
    void saveSettings();
    void printSettings();
};

#endif