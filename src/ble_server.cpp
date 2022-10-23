#include "ble_server.h"
#include "board.h"

#include "atoll_ble.h"

void BleServer::setup(const char *deviceName, ::Preferences *p) {
    Atoll::BleServer::setup(deviceName);
    preferencesSetup(p, "BLE");
    loadSettings();
    printSettings();

    startCpService();
    startCscService();
    startWsService();

    lastPowerNotification = millis();
    lastCadenceNotification = lastPowerNotification;
}

void BleServer::init() {
    uint16_t mtu = 23;  // optimal
    // uint16_t mtu = 64 + 15;
    // uint16_t mtu = BLE_ATT_MTU_MAX;
    // uint16_t mtu = 280;  // api init string fits

    /*
      index name                       IO capability
      0x00  BLE_HS_IO_DISPLAY_ONLY     DisplayOnly
      0x01  BLE_HS_IO_DISPLAY_YESNO    DisplayYesNo
      0x02  BLE_HS_IO_KEYBOARD_ONLY    KeyboardOnly
      0x03  BLE_HS_IO_NO_INPUT_OUTPUT  NoInputNoOutput
      0x04  BLE_HS_IO_KEYBOARD_DISPLAY KeyboardDisplay
    */
    uint8_t iocap = BLE_HS_IO_DISPLAY_ONLY;

    Atoll::Ble::init(deviceName, mtu, iocap);
}

uint16_t BleServer::getAppearance() {
    return APPEARANCE_CYCLING_POWER_SENSOR;
}

void BleServer::loop() {
    if (!enabled) return;
    if (!started) {
        log_e("not started");
        return;
    }
    const ulong t = millis();
    if (powerNotificationReady || lastPowerNotification < t - 1000)
        notifyCp(t);
    if (cadenceNotificationReady || lastCadenceNotification < t - 1500)
        notifyCsc(t);
    if (advertising && !advertising->isAdvertising()) startAdvertising();
    if (lastWmNotification < t - 500) {
        if (wmCharMode == WM_ON ||              //
            (wmCharMode == WM_WHEN_NO_CRANK &&  //
             board.motion.lastCrankEventTime < t - 2000))
            setWmValue(board.strain.liveValue());
        else
            setWmValue(0.0);
        lastWmNotification = t;
    }
    if (hallCharUpdateEnabled && lastHallNotification < t - 300) {
        setHallValue(board.motion.lastHallValue);
        lastHallNotification = t;
    }
}

void BleServer::startCpService() {
    log_i("Starting CPS");
    cpsUUID = BLEUUID(CYCLING_POWER_SERVICE_UUID);
    cps = createService(cpsUUID);

    // Cycling Power Feature
    BLECharacteristic *cpfChar = cps->createCharacteristic(
        BLEUUID(CYCLING_POWER_FEATURE_CHAR_UUID),
        NIMBLE_PROPERTY::READ
        //| NIMBLE_PROPERTY::READ_ENC
    );
    cpfChar->setCallbacks(this);

    bufPowerFeature[0] = 0x00;
    bufPowerFeature[1] = 0x00;
    bufPowerFeature[2] = 0x00;
    if (cadenceInCpm) {
        bufPowerFeature[3] = 0b00001000;  // lsb #3: crank revolution data supported
    } else {
        bufPowerFeature[3] = 0x00;
    }
    cpfChar->setValue((uint8_t *)&bufPowerFeature, 4);

    // Cycling Power Measurement
    cpmChar = cps->createCharacteristic(
        BLEUUID(CYCLING_POWER_MEASUREMENT_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        //| NIMBLE_PROPERTY::INDICATE
    );

    BLEDescriptor *cpmDesc = cpmChar->createDescriptor(
        BLEUUID(CYCLING_POWER_MEASUREMENT_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    char s[SETTINGS_STR_LENGTH];
    strncpy(s, cadenceInCpm ? "Power and cadence measurement" : "Power measurement", sizeof(s));
    cpmDesc->setValue((uint8_t *)s, strlen(s));

    cpmChar->setCallbacks(this);
    cps->start();

    /*
    BLECharacteristic *cpChar = cps->createCharacteristic(
        BLEUUID(SC_CONTROL_POINT_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    bufControlPoint[0] = SC_CONTROL_POINT_OP_SET_CUMULATIVE_VALUE;
    cpChar->setValue((uint8_t *)&bufControlPoint, 1);
    */

    // CPS Sensor Location
    BLECharacteristic *slChar = cps->createCharacteristic(
        BLEUUID(SENSOR_LOCATION_CHAR_UUID),
        NIMBLE_PROPERTY::READ
        //| NIMBLE_PROPERTY::READ_ENC
    );
    slChar->setCallbacks(this);
    bufSensorLocation[0] = SENSOR_LOCATION_RIGHT_CRANK & 0xff;
    slChar->setValue((uint8_t *)&bufSensorLocation, 1);

    advertiseService(cpsUUID);
}

void BleServer::stopCpService() {
    log_i("Stopping CPS");
    unAdvertiseService(cpsUUID);
    removeService(cps);
}

void BleServer::startCscService() {
    if (!cscServiceActive) return;
    log_i("Starting CSCS");
    cscsUUID = BLEUUID(CYCLING_SPEED_CADENCE_SERVICE_UUID);
    cscs = createService(cscsUUID);

    // Cycling Speed and Cadence Feature
    BLECharacteristic *cscfChar = cscs->createCharacteristic(
        BLEUUID(CSC_FEATURE_CHAR_UUID),
        NIMBLE_PROPERTY::READ
        //| NIMBLE_PROPERTY::READ_ENC
    );
    cscfChar->setCallbacks(this);
    bufSpeedCadenceFeature[0] = 0x00;
    // bufSpeedCadenceFeature[1] = 0b00000010;
    bufSpeedCadenceFeature[1] = CSCF_CRANK_REVOLUTION_DATA_SUPPORTED && 0xff;
    cscfChar->setValue((uint8_t *)&bufSpeedCadenceFeature, 2);

    // Cycling Speed and Cadence Measurement
    cscmChar = cscs->createCharacteristic(
        BLEUUID(CSC_MEASUREMENT_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        // NIMBLE_PROPERTY::INDICATE
    );

    BLEDescriptor *cscmDesc = cscmChar->createDescriptor(
        BLEUUID(CSC_MEASUREMENT_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    char s[SETTINGS_STR_LENGTH] = "Cadence measurement";
    cscmDesc->setValue((uint8_t *)s, strlen(s));

    cscmChar->setCallbacks(this);
    cscs->start();
    advertiseService(cscsUUID);
}

void BleServer::stopCscService() {
    log_i("Stopping CSCS");
    unAdvertiseService(cscsUUID);
    removeService(cscs);
}

// Start Weight Scale service
void BleServer::startWsService() {
    log_i("Starting WSS");
    wssUUID = BLEUUID(WEIGHT_SCALE_SERVICE_UUID);
    wss = createService(wssUUID);

    // Weight Scale Feature
    BLECharacteristic *wsfChar = wss->createCharacteristic(
        BLEUUID(WEIGHT_SCALE_FEATURE_UUID),
        NIMBLE_PROPERTY::READ
        //| NIMBLE_PROPERTY::READ_ENC
    );
    wsfChar->setCallbacks(this);
    unsigned char bufWsf[4];
    bufWsf[0] = 0x00;  // No features supported
    bufWsf[1] = 0x00;  // Measurement resolution: not specified
    bufWsf[2] = 0x00;
    bufWsf[3] = 0x00;
    wsfChar->setValue((uint8_t *)&bufWsf, 4);

    // Weight Measurement
    wmChar = wss->createCharacteristic(
        BLEUUID(WEIGHT_MEASUREMENT_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        // NIMBLE_PROPERTY::INDICATE
    );

    BLEDescriptor *wmDesc = wmChar->createDescriptor(
        BLEUUID(WEIGHT_MEASUREMENT_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    char s[SETTINGS_STR_LENGTH] = "Can be enabled in API";
    wmDesc->setValue((uint8_t *)s, strlen(s));

    wmChar->setCallbacks(this);
    wss->start();
    advertiseService(wssUUID);
}

void BleServer::onCrankEvent(const ulong t, const uint16_t revolutions) {
    if (crankRevs < revolutions) {
        crankRevs = revolutions;
        lastCrankEventTime = (uint16_t)(t * 1.024);
        cadenceNotificationReady = true;
        // notifyCsc(t);
    }
    powerNotificationReady = true;
    // notifyCp(t);
}

// notify Cycling Power service
void BleServer::notifyCp(const ulong t) {
    powerNotificationReady = false;
    if (!enabled) {
        log_i("Not enabled, not notifying CP");
        return;
    }
    if (t - CRANK_EVENT_MIN_MS < lastPowerNotification) return;
    lastPowerNotification = t;
    static uint16_t prevPower = 0;
    power = (uint16_t)board.getPower();
    if (power == prevPower) {
        // log_i("Power not changed, not notifying CP");
        return;
    }
    prevPower = power;
    if (cadenceInCpm) {
        bufPower[0] = powerFlagsWithCadence & 0xff;
        bufPower[1] = (powerFlagsWithCadence >> 8) & 0xff;
    } else {
        bufPower[0] = powerFlags & 0xff;
        bufPower[1] = (powerFlags >> 8) & 0xff;
    }
    bufPower[2] = power & 0xff;
    bufPower[3] = (power >> 8) & 0xff;
    if (cadenceInCpm) {
        bufPower[4] = crankRevs & 0xff;
        bufPower[5] = (crankRevs >> 8) & 0xff;
        bufPower[6] = lastCrankEventTime & 0xff;
        bufPower[7] = (lastCrankEventTime >> 8) & 0xff;
        cpmChar->setValue((uint8_t *)&bufPower, 8);
    } else {
        cpmChar->setValue((uint8_t *)&bufPower, 4);
    }
    // log_i("Notifying power %d", power);
    cpmChar->notify();
}

// notify Cycling Speed and Cadence service
void BleServer::notifyCsc(const ulong t) {
    cadenceNotificationReady = false;
    if (!enabled) {
        log_i("Not enabled, not notifying SCS");
        return;
    }
    if (!cscServiceActive) return;
    if (cscmChar == nullptr) return;
    if (t - CRANK_EVENT_MIN_MS < lastCadenceNotification) return;
    lastCadenceNotification = t;
    bufCadence[0] = cadenceFlags & 0xff;
    bufCadence[1] = crankRevs & 0xff;
    bufCadence[2] = (crankRevs >> 8) & 0xff;
    bufCadence[3] = lastCrankEventTime & 0xff;
    bufCadence[4] = (lastCrankEventTime >> 8) & 0xff;
    cscmChar->setValue((uint8_t *)&bufCadence, 5);
    // log_i("Notifying cadence #%d ts %d", crankRevs, t);
    cscmChar->notify();
}

// Notify Battery Level service
// void BleServer::notifyBl(const ulong t) {
//     if (!enabled) {
//         log_i("Not enabled, not notifying BL");
//         return;
//     }

//     if (t - 2000 < lastBatteryNotification) return;
//     lastBatteryLevel = board.battery.level;
//     lastBatteryNotification = t;
//     blChar->setValue(&lastBatteryLevel, 1);
//     blChar->notify();
// }

// Set Weight Measurement char value
void BleServer::setWmValue(float value) {
    if (!enabled) return;
    if (lastWmValue == value) return;
    lastWmValue = value;
    uint8_t flags;
    flags = (uint8_t)0;  // Measurement Units: SI; no additional fields present
    uint16_t measurement;
    // (value/5*1000) https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.characteristic.weight_measurement.xml
    measurement = (uint16_t)(value * 200);
    uint8_t bytes[3];
    bytes[0] = flags;
    bytes[1] = measurement & 0xff;
    bytes[2] = (measurement >> 8) & 0xff;
    wmChar->setValue((uint8_t *)bytes, 3);
    wmChar->notify();
}

// Set Hall Effect Sensor Measurement char value
void BleServer::setHallValue(int value) {
    if (!enabled) return;
    uint8_t bytes[2];
    bytes[0] = value & 0xff;
    bytes[1] = (value >> 8) & 0xff;
    hallChar->setValue((uint8_t *)bytes, 2);
    hallChar->notify();
}

const char *BleServer::characteristicStr(BLECharacteristic *c) {
    if (c == nullptr) return "unknown characteristic";
    if (cpmChar != nullptr && cpmChar->getHandle() == c->getHandle()) return "CPM";
    if (cscmChar != nullptr && cscmChar->getHandle() == c->getHandle()) return "CSCM";
    if (wmChar != nullptr && wmChar->getHandle() == c->getHandle()) return "WM";
    if (hallChar != nullptr && hallChar->getHandle() == c->getHandle()) return "HALL";
    return c->getUUID().toString().c_str();
}

void BleServer::setCadenceInCpm(bool state) {
    if (state == cadenceInCpm) return;
    cadenceInCpm = state;
    saveSettings();
    stopCpService();
    startCpService();
}

void BleServer::setCscServiceActive(bool state) {
    if (state == cscServiceActive) return;
    cscServiceActive = state;
    saveSettings();
    if (cscServiceActive)
        startCscService();
    else
        stopCscService();
}

// Set the operating mode of the Weight Measurement char
void BleServer::setWmCharMode(uint8_t mode) {
    wmCharMode = mode;
}

// Set the "update enabled" flag on the Hall char
void BleServer::setHallCharUpdateEnabled(bool state) {
    hallCharUpdateEnabled = state;
}

void BleServer::loadSettings() {
    if (!preferencesStartLoad()) return;
    cadenceInCpm = preferences->getBool("cadenceInCpm", cadenceInCpm);
    cscServiceActive = preferences->getBool("cscService", cscServiceActive);
    preferencesEnd();
}

void BleServer::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putBool("cadenceInCpm", cadenceInCpm);
    preferences->putBool("cscService", cscServiceActive);
    preferencesEnd();
}

void BleServer::printSettings() {
    log_i("Cadence data in CPM: %s", cadenceInCpm ? "Yes" : "No");
    log_i("CSC service: %s", cscServiceActive ? "Active" : "Not active");
}

void BleServer::onConnect(BLEServer *pServer, BLEConnInfo &info) {
    // if (board.api.secureBle) {
    //     log_i("calling startSecurity()");
    //     int res = BLEDevice::startSecurity(desc->conn_handle);
    //     if (0 != res) {
    //         log_e("startSecurity() returned %d", res);
    //     }
    // }
    Atoll::BleServer::onConnect(pServer, info);
}