#include "ble.h"
#include "board.h"

void BLE::setup(const char *deviceName, Preferences *p) {
    strncpy(this->deviceName, deviceName, sizeof(this->deviceName));
    preferencesSetup(p, "BLE");
    loadSettings();
    printSettings();
    enabled = true;
    BLEDevice::init(deviceName);

    if (secureApi) {
        NimBLEDevice::setSecurityAuth(true, true, true);
        NimBLEDevice::setSecurityPasskey(passkey);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    }

    server = BLEDevice::createServer();
    server->setCallbacks(this);
    advertising = server->getAdvertising();
    advertising->setAppearance(APPEARANCE_CYCLING_POWER_SENSOR);
    //advertising->setManufacturerData("G");
    //advertising->setScanResponse(false);
    //advertising->setMinPreferred(0x0);

    startDiService();
    startCpService();
    startCscService();
    startBlService();
    startWsService();
    startApiService();

    server->start();
    lastPowerNotification = millis();
    lastCadenceNotification = lastPowerNotification;
    lastBatteryNotification = lastPowerNotification;
}

void BLE::loop() {
    if (!enabled) return;
    const ulong t = millis();
    if (powerNotificationReady || lastPowerNotification < t - 1000)
        notifyCp(t);
    if (cadenceNotificationReady || lastCadenceNotification < t - 1500)
        notifyCsc(t);
    if (lastBatteryLevel != board.battery.level) notifyBl(t);
    if (!advertising->isAdvertising()) startAdvertising();
    if (wmCharUpdateEnabled && lastWmNotification < t - 200) {
        setWmValue(board.strain.liveValue());
        lastWmNotification = t;
    }
    if (hallCharUpdateEnabled && lastHallNotification < t - 200) {
        setHallValue(board.motion.lastHallValue);
        lastHallNotification = t;
    }
}

// Start Device Information service
void BLE::startDiService() {
    Serial.println("[BLE] Starting DIS");
    disUUID = BLEUUID(DEVICE_INFORMATION_SERVICE_UUID);
    dis = server->createService(disUUID);
    diChar = dis->createCharacteristic(
        BLEUUID(DEVICE_NAME_CHAR_UUID),
        NIMBLE_PROPERTY::READ
        //| NIMBLE_PROPERTY::NOTIFY
        //| NIMBLE_PROPERTY::INDICATE
    );
    diChar->setValue((uint8_t *)&deviceName, strlen(deviceName));
    diChar->setCallbacks(this);
    dis->start();
    advertising->addServiceUUID(disUUID);
}

void BLE::startCpService() {
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

    advertising->addServiceUUID(cpsUUID);
}

void BLE::stopCpService() {
    Serial.println("[BLE] Stopping CPS");
    advertising->removeServiceUUID(cpsUUID);
    server->removeService(cps, true);
}

void BLE::startCscService() {
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
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        //NIMBLE_PROPERTY::INDICATE
    );

    BLEDescriptor *cscmDesc = cscmChar->createDescriptor(
        BLEUUID(CSC_MEASUREMENT_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    char s[SETTINGS_STR_LENGTH] = "Cadence measurement";
    cscmDesc->setValue((uint8_t *)s, strlen(s));

    cscmChar->setCallbacks(this);
    cscs->start();
    advertising->addServiceUUID(cscsUUID);
}

void BLE::stopCscService() {
    Serial.println("[BLE] Stopping CSCS");
    advertising->removeServiceUUID(cscsUUID);
    server->removeService(cscs, true);
}

// Start Battey Level service
void BLE::startBlService() {
    Serial.println("[BLE] Starting BLS");
    blsUUID = BLEUUID(BATTERY_SERVICE_UUID);
    bls = server->createService(blsUUID);
    blChar = bls->createCharacteristic(
        BLEUUID(BATTERY_LEVEL_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        //| NIMBLE_PROPERTY::INDICATE
    );
    blChar->setCallbacks(this);
    BLEDescriptor *blDesc = blChar->createDescriptor(
        BLEUUID(BATTERY_LEVEL_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    char s[SETTINGS_STR_LENGTH] = "Percentage";
    blDesc->setValue((uint8_t *)s, strlen(s));
    bls->start();
    advertising->addServiceUUID(blsUUID);
}

// Start Weight Scale service
void BLE::startWsService() {
    Serial.println("[BLE] Starting WSS");
    wssUUID = BLEUUID(WEIGHT_SCALE_SERVICE_UUID);
    wss = server->createService(wssUUID);

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
        //NIMBLE_PROPERTY::INDICATE
    );

    BLEDescriptor *wmDesc = wmChar->createDescriptor(
        BLEUUID(WEIGHT_MEASUREMENT_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    char s[SETTINGS_STR_LENGTH] = "Can be enabled in API";
    wmDesc->setValue((uint8_t *)s, strlen(s));

    wmChar->setCallbacks(this);
    wss->start();
    advertising->addServiceUUID(wssUUID);
}

void BLE::startApiService() {
    Serial.println("[BLE] Starting APIS");
    char s[SETTINGS_STR_LENGTH] = "";
    asUUID = BLEUUID(API_SERVICE_UUID);
    as = server->createService(asUUID);

    // api char for writing commands and reading responses
    uint32_t properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE;
    if (secureApi)
        properties |= NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN;
    apiChar = as->createCharacteristic(BLEUUID(API_CHAR_UUID), properties);
    apiChar->setCallbacks(this);
    strncpy(s, "Ready", 32);
    apiChar->setValue((uint8_t *)s, strlen(s));
    BLEDescriptor *apiDesc = apiChar->createDescriptor(
        BLEUUID(API_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    strncpy(s, "ESPM API v0.1", 32);
    apiDesc->setValue((uint8_t *)s, strlen(s));

    // api char for reading hall effect sensor measurements
    hallChar = as->createCharacteristic(
        BLEUUID(HALL_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::NOTIFY);
    hallChar->setCallbacks(this);
    uint8_t bytes[2];
    bytes[0] = 0 & 0xff;
    bytes[1] = (0 >> 8) & 0xff;
    hallChar->setValue((uint8_t *)bytes, 2);  // set initial value
    BLEDescriptor *hallDesc = hallChar->createDescriptor(
        BLEUUID(HALL_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    strncpy(s, "Hall Effect Sensor reading", 32);
    hallDesc->setValue((uint8_t *)s, strlen(s));

    as->start();
    advertising->addServiceUUID(asUUID);
}

void BLE::onCrankEvent(const ulong t, const uint16_t revolutions) {
    if (crankRevs < revolutions) {
        crankRevs = revolutions;
        lastCrankEventTime = (uint16_t)(t * 1.024);
        cadenceNotificationReady = true;
        //notifyCsc(t);
    }
    powerNotificationReady = true;
    //notifyCp(t);
}

// notify Cycling Power service
void BLE::notifyCp(const ulong t) {
    powerNotificationReady = false;
    if (!enabled) {
        Serial.println("[BLE] Not enabled, not notifying CP");
        return;
    }
    if (t - CRANK_EVENT_MIN_MS < lastPowerNotification) return;
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

// notify Cycling Speed and Cadence service
void BLE::notifyCsc(const ulong t) {
    cadenceNotificationReady = false;
    if (!enabled) {
        Serial.println("[BLE] Not enabled, not notifying SCS");
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
    //Serial.printf("[BLE] Notifying cadence #%d ts %d\n", crankRevs, t);
    cscmChar->notify();
}

// Notify Battery Level service
void BLE::notifyBl(const ulong t) {
    if (!enabled) {
        Serial.println("[BLE] Not enabled, not notifying BL");
        return;
    }

    if (t - 2000 < lastBatteryNotification) return;
    lastBatteryLevel = board.battery.level;
    lastBatteryNotification = t;
    blChar->setValue(&lastBatteryLevel, 1);
    blChar->notify();
}

// response format: responseCode:responseStr;commandCode:commandStr=[arg]
void BLE::handleApiCommand(const char *command) {
    char reply[API_REPLY_MAXLENGTH] = "";
    API::Result result = board.api.handleCommand(command, reply);
    char response[BLE_CHAR_VALUE_MAXLENGTH] = "";
    snprintf(response, sizeof(response), "%d:%s;%s",
             (int)result, board.api.resultStr(result), reply);
    Serial.printf("[BLE] handleApiCommand(\"%s\") response: %s\n", command, response);
    setApiValue(response);
    if (API::Result::success == result)
        board.led.blink(2, 100, 300);
    else
        board.led.blink(10, 100, 100);
}

void BLE::setApiValue(const char *value) {
    if (!enabled) {
        Serial.println("[BLE] Not enabled, not setting API value");
        return;
    }
    apiChar->setValue((uint8_t *)value, strlen(value));
    apiChar->notify();
}

// Set Weight Measurement char value
void BLE::setWmValue(float value) {
    if (!enabled) return;
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
void BLE::setHallValue(int value) {
    if (!enabled) return;
    uint8_t bytes[2];
    bytes[0] = value & 0xff;
    bytes[1] = (value >> 8) & 0xff;
    hallChar->setValue((uint8_t *)bytes, 2);
    hallChar->notify();
}

const char *BLE::characteristicStr(BLECharacteristic *c) {
    if (c == nullptr) return "unknown characteristic";
    if (cpmChar != nullptr && cpmChar->getHandle() == c->getHandle()) return "CPM";
    if (cscmChar != nullptr && cscmChar->getHandle() == c->getHandle()) return "CSCM";
    if (blChar != nullptr && blChar->getHandle() == c->getHandle()) return "BL";
    if (apiChar != nullptr && apiChar->getHandle() == c->getHandle()) return "API";
    if (wmChar != nullptr && wmChar->getHandle() == c->getHandle()) return "WM";
    if (hallChar != nullptr && hallChar->getHandle() == c->getHandle()) return "HALL";
    return c->getUUID().toString().c_str();
}

// disconnect clients, stop advertising and shutdown BLE
void BLE::stop() {
    while (!_clients.isEmpty())
        server->disconnect(_clients.shift());
    server->stopAdvertising();
    enabled = false;
    delay(100);  // give the BLE stack a chance to clear packets
    //BLEDevice::deinit(true);  // TODO never returns
}

void BLE::onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) {
    Serial.printf("[BLE] Client connected, ID: %d Address: %s\n",
                  desc->conn_handle,
                  BLEAddress(desc->peer_ota_addr).toString().c_str());
    //NimBLEDevice::startSecurity(desc->conn_handle);
    // save client handle so we can gracefully disconnect them
    bool savedClientHandle = false;
    for (decltype(_clients)::index_t i = 0; i < _clients.size(); i++)
        if (_clients[i] == desc->conn_handle)
            savedClientHandle = true;
    if (!savedClientHandle)
        _clients.push(desc->conn_handle);
}

void BLE::onDisconnect(BLEServer *pServer) {
    Serial.println("[BLE] Server onDisconnect");
}

void BLE::startAdvertising() {
    if (!enabled) {
        Serial.println("[BLE] Not enabled, not starting advertising");
        return;
    }
    delay(300);
    if (!advertising->isAdvertising()) {
        server->startAdvertising();
        Serial.println("[BLE] Start advertising");
    }
}

void BLE::onRead(BLECharacteristic *c) {
    Serial.printf("[BLE] %s: onRead(), value: %s\n",
                  characteristicStr(c),
                  c->getValue().c_str());
};

void BLE::onWrite(BLECharacteristic *c) {
    char value[BLE_CHAR_VALUE_MAXLENGTH] = "";
    strncpy(value, c->getValue().c_str(), sizeof(value));
    Serial.printf("[BLE] %s: onWrite(), value: %s\n",
                  characteristicStr(c),
                  value);
    if (c->getHandle() == apiChar->getHandle())
        handleApiCommand(value);
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

void BLE::setCadenceInCpm(bool state) {
    if (state == cadenceInCpm) return;
    cadenceInCpm = state;
    saveSettings();
    stopCpService();
    startCpService();
}

void BLE::setCscServiceActive(bool state) {
    if (state == cscServiceActive) return;
    cscServiceActive = state;
    saveSettings();
    if (cscServiceActive)
        startCscService();
    else
        stopCscService();
}

void BLE::setSecureApi(bool state) {
    if (state == secureApi) return;
    secureApi = state;
    saveSettings();
    Serial.printf("[BLE] SecureAPI %sabled\n", secureApi ? "en" : "dis");
    /* TODO deinit() does not return
    stop();
    setup(deviceName, preferences);
    */
}

void BLE::setPasskey(uint32_t newPasskey) {
    if (newPasskey == passkey) return;
    passkey = newPasskey;
    saveSettings();
    Serial.printf("[BLE] New passkey: %d\n", passkey);
    /* TODO deinit() does not return
    stop();
    setup(deviceName, preferences);
    */
}

// Set the "update enabled" flag on the Weight Measurement char
void BLE::setWmCharUpdateEnabled(bool state) {
    wmCharUpdateEnabled = state;
}

// Set the "update enabled" flag on the Hall char
void BLE::setHallCharUpdateEnabled(bool state) {
    hallCharUpdateEnabled = state;
}

void BLE::loadSettings() {
    if (!preferencesStartLoad()) return;
    cadenceInCpm = preferences->getBool("cadenceInCpm", cadenceInCpm);
    cscServiceActive = preferences->getBool("cscService", cscServiceActive);
    secureApi = preferences->getBool("secureApi", secureApi);
    passkey = (uint32_t)preferences->getInt("passkey", passkey);
    preferencesEnd();
}

void BLE::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putBool("cadenceInCpm", cadenceInCpm);
    preferences->putBool("cscService", cscServiceActive);
    preferences->putBool("secureApi", secureApi);
    preferences->putInt("passkey", (int32_t)passkey);
    preferencesEnd();
}

void BLE::printSettings() {
    Serial.printf("[BLE] Cadence data in CPM: %s\n", cadenceInCpm ? "Yes" : "No");
    Serial.printf("[BLE] CSC service: %s\n", cscServiceActive ? "Active" : "Not active");
    Serial.printf("[BLE] SecureAPI: %s\n", secureApi ? "Yes" : "No");
    Serial.printf("[BLE] Passkey: %d\n", passkey);
}