#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <CircularBuffer.h>

#include "definitions.h"
#include "ble_constants.h"
#include "haspreferences.h"
#include "task.h"

#define BLE_CHAR_VALUE_MAXLENGTH 128

class BLE : public Task,
            public HasPreferences,
            public BLEServerCallbacks,
            public BLECharacteristicCallbacks {
   public:
    char deviceName[SETTINGS_STR_LENGTH] = HOSTNAME;  // advertised device name
    bool enabled = true;                              // whether bluetooth is enabled
    BLEServer *server;                                // pointer to the ble server
    BLEUUID disUUID;                                  // device information service uuid
    BLEService *dis;                                  // device information service
    BLECharacteristic *diChar;                        // device information characteristic
    BLEUUID cpsUUID;                                  // cycling power service uuid
    BLEService *cps;                                  // cycling power service
    BLECharacteristic *cpmChar;                       // cycling power measurement characteristic
    BLEUUID cscsUUID;                                 // cycling speed and cadence service uuid
    BLEService *cscs;                                 // cycling speed and cadence service
    BLECharacteristic *cscmChar;                      // cycling speed and cadence measurement characteristic
    BLEUUID blsUUID;                                  // battery level service uuid
    BLEService *bls;                                  // battery level service
    BLECharacteristic *blChar;                        // battery level characteristic
    BLEUUID wssUUID;                                  // weight scale service uuid
    BLEService *wss;                                  // weight scale service
    BLECharacteristic *wmChar;                        // weight measurement characteristic
    BLEUUID asUUID;                                   // api service uuid
    BLEService *as;                                   // api service
    BLECharacteristic *apiChar;                       // api characteristic
    BLEAdvertising *advertising;                      // pointer to advertising

    bool powerNotificationReady = false;
    unsigned long lastPowerNotification = 0;
    bool cadenceNotificationReady = false;
    unsigned long lastCadenceNotification = 0;
    uint8_t lastBatteryLevel = 0;
    unsigned long lastBatteryNotification = 0;
    bool wmCharUpdateEnabled = false;  // enables weight measurement value updates and notifications
    unsigned long lastWmNotification = 0;

    bool cadenceInCpm = true;       // whether to include cadence data in CPM
    bool cscServiceActive = false;  // whether CSC service should be active
    bool secureApi = false;         // whether to use LESC for API service
    uint32_t passkey = 696669;      // passkey for API service, max 6 digits

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
    void startDiService();
    void startCpService();
    void stopCpService();
    void startCscService();
    void stopCscService();
    void startBlService();
    void startWsService();
    void startApiService();
    void onCrankEvent(const ulong t, const uint16_t revolutions);
    void notifyCp(const ulong t);
    void notifyCsc(const ulong t);
    void notifyBl(const ulong t);
    void handleApiCommand(const char *command);
    void setApiValue(const char *value);
    void setWmValue(float value);
    const char *characteristicStr(BLECharacteristic *c);
    void stop();

    void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc);
    void onDisconnect(BLEServer *pServer);
    void startAdvertising();

    void onRead(BLECharacteristic *pCharacteristic);
    void onWrite(BLECharacteristic *pCharacteristic);
    void onNotify(BLECharacteristic *pCharacteristic);
    void onSubscribe(BLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue);

    void setCadenceInCpm(bool state);
    void setCscServiceActive(bool state);
    void setSecureApi(bool state);
    void setPasskey(uint32_t newPasskey);
    void setWmCharUpdateEnabled(bool state);
    void loadSettings();
    void saveSettings();
    void printSettings();

   private:
    CircularBuffer<uint16_t, 32> _clients;  // keeps track of connected client handles, in order to gracefully disconnect them before deep sleep or reboot; TODO add has() to CircularBuffer
};

#endif