#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include "definitions.h"
#include "atoll_ble_server.h"
#include "atoll_preferences.h"

#ifndef BLE_CHAR_VALUE_MAXLENGTH
#define BLE_CHAR_VALUE_MAXLENGTH 128
#endif

#ifndef BLE_APPEARANCE
#define BLE_APPEARANCE APPEARANCE_CYCLING_POWER_SENSOR
#endif

class BleServer : public Atoll::BleServer,
                  public Atoll::Preferences {
   public:
    // BLEUUID disUUID;              // device information service uuid
    // BLEService *dis;              // device information service
    // BLECharacteristic *diChar;    // device information characteristic
    BLEUUID cpsUUID;              // cycling power service uuid
    BLEService *cps;              // cycling power service
    BLECharacteristic *cpmChar;   // cycling power measurement characteristic
    BLEUUID cscsUUID;             // cycling speed and cadence service uuid
    BLEService *cscs;             // cycling speed and cadence service
    BLECharacteristic *cscmChar;  // cycling speed and cadence measurement characteristic
    // BLEUUID blsUUID;              // battery level service uuid
    // BLEService *bls;              // battery level service
    // BLECharacteristic *blChar;    // battery level characteristic
    BLEUUID wssUUID;            // weight scale service uuid
    BLEService *wss;            // weight scale service
    BLECharacteristic *wmChar;  // weight measurement characteristic
    // BLEUUID asUUID;               // api service uuid
    // BLEService *as;               // api service
    // BLECharacteristic *apiChar;   // api characteristic
    BLECharacteristic *hallChar;  // hall effect sensor measurement characteristic
    // BLEAdvertising *advertising;  // pointer to advertising

    bool powerNotificationReady = false;
    unsigned long lastPowerNotification = 0;
    bool cadenceNotificationReady = false;
    unsigned long lastCadenceNotification = 0;
    // uint8_t lastBatteryLevel = 0;
    // unsigned long lastBatteryNotification = 0;
    uint8_t wmCharMode = WM_CHAR_MODE;   // weight measurement char updates and notifications
    bool hallCharUpdateEnabled = false;  // enables hall measurement value updates and notifications
    unsigned long lastWmNotification = 0;
    float lastWmValue = 0.0;
    unsigned long lastHallNotification = 0;

    bool cadenceInCpm = true;       // whether to include cadence data in CPM
    bool cscServiceActive = false;  // whether CSC service should be active

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

    void setup(const char *deviceName, ::Preferences *p);
    void loop();
    void startCpService();
    void stopCpService();
    void startCscService();
    void stopCscService();
    void startWsService();
    void onCrankEvent(const ulong t, const uint16_t revolutions);
    void notifyCp(const ulong t);
    void notifyCsc(const ulong t);
    // void notifyBl(const ulong t);
    void setWmValue(float value);
    void setHallValue(int value);
    const char *characteristicStr(BLECharacteristic *c);

    void setCadenceInCpm(bool state);
    void setCscServiceActive(bool state);
    void setWmCharMode(uint8_t mode);
    void setHallCharUpdateEnabled(bool state);
    void loadSettings();
    void saveSettings();
    void printSettings();
};

#endif