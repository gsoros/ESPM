#include "ble.h"
#include "board.h"

void BLE::setup(const char *deviceName, Preferences *p) {
    preferencesSetup(p, "BLE");
    loadSettings();
    printSettings();
    BLEDevice::init(deviceName);
    //NimBLEDevice::setSecurityAuth(false, false, false);
    //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    //NimBLEDevice::setSecurityPasskey(0);
    server = BLEDevice::createServer();
    server->setCallbacks(this);
    advertising = server->getAdvertising();
    advertising->setAppearance(APPEARANCE_CYCLING_POWER_SENSOR);
    //advertising->setManufacturerData("G");
    //advertising->setScanResponse(false);
    //advertising->setMinPreferred(0x0);

    startPowerService();
    startCadenceService();
    startBatterySerice();
    startApiSerice();

    server->start();
    lastPowerNotification = millis();
    lastCadenceNotification = lastPowerNotification;
    lastBatteryNotification = lastPowerNotification;
}

void BLE::loop() {
    const ulong t = millis();
    if (lastPowerNotification < t - 1000) notifyPower(t);
    if (lastCadenceNotification < t - 1500) notifyCadence(t);
    if (lastBatteryLevel != board.battery.level) {
        notifyBattery(t);
    }
    if (!server->getAdvertising()->isAdvertising())
        startAdvertising();
}

void BLE::startPowerService() {
    Serial.println("[BLE] Starting CPS");
    cpsUUID = BLEUUID(CYCLING_POWER_SERVICE_UUID);
    cps = server->createService(cpsUUID);

    // Cycling Power Feature
    BLECharacteristic *cpfChar = cps->createCharacteristic(
        BLEUUID(CYCLING_POWER_FEATURE_CHAR_UUID),
        NIMBLE_PROPERTY::READ
        //| NIMBLE_PROPERTY::READ_ENC
    );
    cpfChar->setCallbacks(this);

    /*
    // This is incorrect
    // TODO find out how to set the bytes
    uint32_t powerFeature = (uint32_t)0x00;
    if (cadenceInCpm) { 
        powerFeature = powerFeature | CPF_CRANK_REVOLUTION_DATA_SUPPORTED;
    }
    cpfChar->setValue((uint8_t *)&powerFeature, 4);
    */

    // Correct but ugly
    bufPowerFeature[0] = 0x00;
    bufPowerFeature[1] = 0x00;
    bufPowerFeature[2] = 0x00;
    if (cadenceInCpm) {
        bufPowerFeature[3] = 0b00001000;  // lsb #3: crank revolution data supported
    } else {
        bufPowerFeature[3] = 0x00;
    }
    cpfChar->setValue((uint8_t *)&bufPowerFeature, 4);

    // Cycling Power Mmeasurement
    cpmChar = cps->createCharacteristic(
        BLEUUID(CYCLING_POWER_MEASUREMENT_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::NOTIFY);
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

    advertising->addServiceUUID(cpsUUID);
}

void BLE::stopPowerService() {
    Serial.println("[BLE] Stopping CPS");
    advertising->removeServiceUUID(cpsUUID);
    server->removeService(cps, true);
}

void BLE::startCadenceService() {
    if (!cscServiceActive) return;
    Serial.println("[BLE] Starting CSCS");
    cscsUUID = BLEUUID(CYCLING_SPEED_CADENCE_SERVICE_UUID);
    cscs = server->createService(cscsUUID);

    // Cycling Speed and Cadence Feature
    BLECharacteristic *cscfChar = cscs->createCharacteristic(
        BLEUUID(CSC_FEATURE_CHAR_UUID),
        NIMBLE_PROPERTY::READ
        //| NIMBLE_PROPERTY::READ_ENC
    );
    cscfChar->setCallbacks(this);
    bufSpeedCadenceFeature[0] = 0x00;
    //bufSpeedCadenceFeature[1] = 0b00000010;
    bufSpeedCadenceFeature[1] = CSCF_CRANK_REVOLUTION_DATA_SUPPORTED && 0xff;
    cscfChar->setValue((uint8_t *)&bufSpeedCadenceFeature, 2);

    // Cycling Speed and Cadence Measurement
    cscmChar = cscs->createCharacteristic(
        BLEUUID(CSC_MEASUREMENT_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::NOTIFY);
    cscmChar->setCallbacks(this);
    cscs->start();
    advertising->addServiceUUID(cscsUUID);
}

void BLE::stopCadenceService() {
    Serial.println("[BLE] Stopping CSCS");
    advertising->removeServiceUUID(cscsUUID);
    server->removeService(cscs, true);
}

void BLE::startBatterySerice() {
    Serial.println("[BLE] Starting BS");
    blsUUID = BLEUUID(BATTERY_SERVICE_UUID);
    bls = server->createService(blsUUID);
    blChar = bls->createCharacteristic(
        BLEUUID(BATTERY_LEVEL_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::NOTIFY);
    blChar->setCallbacks(this);
    BLEDescriptor *blDesc = blChar->createDescriptor(
        BLEUUID(BATTERY_LEVEL_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    blDesc->setValue("Percentage");
    bls->start();
    advertising->addServiceUUID(blsUUID);
}

void BLE::startApiSerice() {
    Serial.println("[BLE] Starting APIS");
    char s[32] = "";
    asUUID = BLEUUID(API_SERVICE_UUID);
    as = server->createService(asUUID);
    apiChar = as->createCharacteristic(
        BLEUUID(API_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE
        //| NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC
    );
    apiChar->setCallbacks(this);
    strncpy(s, "Ready", 32);
    apiChar->setValue((uint8_t *)s, strlen(s));
    BLEDescriptor *apiDesc = apiChar->createDescriptor(
        BLEUUID(API_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    strncpy(s, "ESPM API v0.1", 32);
    apiDesc->setValue((uint8_t *)s, strlen(s));
    as->start();
    advertising->addServiceUUID(asUUID);
}

void BLE::onCrankEvent(const ulong t, const uint16_t revolutions) {
    if (crankRevs < revolutions) {
        crankRevs = revolutions;
        lastCrankEventTime = (uint16_t)(t * 1.024);
        notifyCadence(t);
    }
    notifyPower(t);
}

void BLE::notifyPower(const ulong t) {
    if (t - 300 < lastPowerNotification) return;
    lastPowerNotification = t;
    power = (uint16_t)board.getPower();
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
    //Serial.printf("[BLE] Notifying power %d\n", power);
    cpmChar->notify();
}

void BLE::notifyCadence(const ulong t) {
    if (!cscServiceActive) return;
    if (cscmChar == nullptr) return;
    if (t - 300 < lastCadenceNotification) return;
    lastCadenceNotification = t;
    bufCadence[0] = cadenceFlags & 0xff;
    bufCadence[1] = crankRevs & 0xff;
    bufCadence[2] = (crankRevs >> 8) & 0xff;
    bufCadence[3] = lastCrankEventTime & 0xff;
    bufCadence[4] = (lastCrankEventTime >> 8) & 0xff;
    cscmChar->setValue((uint8_t *)&bufCadence, 5);
    //Serial.printf("[BLE] Notifying cadence #%d ts %d\n", crankRevs, t);
    cscmChar->notify();
}

void BLE::notifyBattery(const ulong t) {
    if (t - 2000 < lastBatteryNotification) return;
    lastBatteryLevel = board.battery.level;
    lastBatteryNotification = t;
    blChar->setValue(&lastBatteryLevel, 1);
    blChar->notify();
}

void BLE::setApiValue(const char *response) {
    apiChar->setValue((uint8_t *)response, strlen(response));
    apiChar->notify();
}

const char *BLE::characteristicStr(BLECharacteristic *c) {
    if (c == nullptr) return "unknown characteristic";
    if (cpmChar != nullptr && cpmChar->getHandle() == c->getHandle()) return "CPM";
    if (cscmChar != nullptr && cscmChar->getHandle() == c->getHandle()) return "CSCM";
    if (blChar != nullptr && blChar->getHandle() == c->getHandle()) return "BL";
    if (apiChar != nullptr && apiChar->getHandle() == c->getHandle()) return "API";
    return c->getUUID().toString().c_str();
}

void BLE::stop() {
    while (!_clients.isEmpty())
        server->disconnect(_clients.shift());
    server->stopAdvertising();
}

bool BLE::_hasClient(uint16_t handle) {
    for (decltype(_clients)::index_t i = 0; i < _clients.size(); i++)
        if (_clients[i] == handle) return true;
    return false;
}

void BLE::onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) {
    Serial.printf("[BLE] Client connected, ID: %d Address: %s\n",
                  desc->conn_handle,
                  BLEAddress(desc->peer_ota_addr).toString().c_str());
    //NimBLEDevice::startSecurity(desc->conn_handle);
    if (!_hasClient(desc->conn_handle)) _clients.push(desc->conn_handle);
}

void BLE::onDisconnect(BLEServer *pServer) {
    Serial.println("[BLE] Server onDisconnect");
}

void BLE::startAdvertising() {
    delay(300);
    Serial.println("[BLE] Start advertising");
    server->startAdvertising();
}

void BLE::onRead(BLECharacteristic *c) {
    Serial.printf("[BLE] %s: onRead(), value: %s\n",
                  characteristicStr(c),
                  c->getValue().c_str());
};

void BLE::onWrite(BLECharacteristic *c) {
    char value[BLE_CHAR_VALUE_MAXLENGTH] = "";
    strncpy(value, c->getValue().c_str(), BLE_CHAR_VALUE_MAXLENGTH);
    Serial.printf("[BLE] %s: onWrite(), value: %s\n",
                  characteristicStr(c),
                  value);
    if (c->getHandle() == apiChar->getHandle()) {
        api_result_t result = board.api.handleCommand(value);
        char response[BLE_CHAR_VALUE_MAXLENGTH] = "";
        snprintf(response, BLE_CHAR_VALUE_MAXLENGTH, "%d:%s; %s", result, board.api.resultStr(result), value);
        Serial.printf("[BLE] response: %s\n", response);
        setApiValue(response);
        if (AR_SUCCESS == result)
            board.led.blink(2, 100, 300);
        else
            board.led.blink(10, 100, 100);
    }
};

void BLE::onNotify(BLECharacteristic *pCharacteristic){
    //Serial.printf("[BLE] Sending notification: %d\n", pCharacteristic->getValue<int>());
};

void BLE::onSubscribe(BLECharacteristic *c, ble_gap_conn_desc *desc, uint16_t subValue) {
    Serial.printf("[BLE] Client ID: %d Address: %s ",
                  desc->conn_handle,
                  BLEAddress(desc->peer_ota_addr).toString().c_str());
    if (subValue == 0)
        Serial.print("unsubscribed from ");
    else if (subValue == 1)
        Serial.print("subscribed to notfications for ");
    else if (subValue == 2)
        Serial.print("Subscribed to indications for ");
    else if (subValue == 3)
        Serial.print("subscribed to notifications and indications for ");
    Serial.println(characteristicStr(c));
};

//bool BLE::onConfirmPIN(uint32_t pin) { Serial.println("onConfirmPIN"); return true; }
//uint32_t BLE::onPassKeyRequest() {Serial.println("onPassKeyRequest");  return 0; }
//void BLE::onAuthenticationComplete(ble_gap_conn_desc *desc) { Serial.println("onAuthenticationComplete"); }

void BLE::setCadenceInCpm(bool state) {
    if (cadenceInCpm == state) return;
    cadenceInCpm = state;
    saveSettings();
    stopPowerService();
    startPowerService();
}

void BLE::setCscServiceActive(bool state) {
    if (cscServiceActive == state) return;
    cscServiceActive = state;
    saveSettings();
    if (cscServiceActive)
        startCadenceService();
    else
        stopCadenceService();
}

void BLE::loadSettings() {
    if (!preferencesStartLoad()) return;
    cadenceInCpm = preferences->getBool("cadenceInCpm", false);
    cscServiceActive = preferences->getBool("cscService", true);
    preferencesEnd();
}

void BLE::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putBool("cadenceInCpm", cadenceInCpm);
    preferences->putBool("cscService", cscServiceActive);
    preferencesEnd();
}

void BLE::printSettings() {
    Serial.printf(
        "[BLE] Settings\nAdd cadence data to CPM: %s\nCSC service active: %s\n",
        cadenceInCpm ? "Yes" : "No",
        cscServiceActive ? "Yes" : "No");
}