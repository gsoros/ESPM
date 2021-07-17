#include "ble.h"
#include "board.h"

void BLE::setup(const char *deviceName) {
    BLEDevice::init(deviceName);
    //NimBLEDevice::setSecurityAuth(false, false, false);
    //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    //NimBLEDevice::setSecurityPasskey(0);
    server = BLEDevice::createServer();
    server->setCallbacks(this);

    // Cycling Power Service
    BLEUUID cpsUUID = BLEUUID(CYCLING_POWER_SERVICE_UUID);
    BLEService *cps = server->createService(cpsUUID);

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
    bufPowerFeature[3] = 0x00;
    cpfChar->setValue((uint8_t *)&bufPowerFeature, 4);

    // Cycling Power Mmeasurement
    cpmChar = cps->createCharacteristic(
        BLEUUID(CYCLING_POWER_MEASUREMENT_CHAR_UUID),
        NIMBLE_PROPERTY::READ
            //| NIMBLE_PROPERTY::READ_ENC
            //| NIMBLE_PROPERTY::WRITE
            | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE);
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

    // Cycling Speed and Cadence
    BLEUUID cscUUID = BLEUUID(CYCLING_SPEED_CADENCE_SERVICE_UUID);
    BLEService *csc = server->createService(cscUUID);

    // Cycling Speed and Cadence Feature
    BLECharacteristic *cscfChar = csc->createCharacteristic(
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
    cscmChar = csc->createCharacteristic(
        BLEUUID(CSC_MEASUREMENT_CHAR_UUID),
        NIMBLE_PROPERTY::READ
            //| NIMBLE_PROPERTY::READ_ENC
            //| NIMBLE_PROPERTY::WRITE
            | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE);
    cscmChar->setCallbacks(this);
    csc->start();

    // Battery Service
    BLEUUID bsUUID = BLEUUID(BATTERY_SERVICE_UUID);
    BLEService *bs = server->createService(bsUUID);
    blChar = bs->createCharacteristic(
        BLEUUID(BATTERY_LEVEL_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    blChar->setCallbacks(this);
    BLEDescriptor *blDesc = blChar->createDescriptor(BLEUUID(BATTERY_LEVEL_DESC_UUID));
    blDesc->setValue("Percentage");
    bs->start();

    BLEAdvertising *advertising = server->getAdvertising();
    advertising->setAppearance(1156);
    advertising->setManufacturerData("G");
    advertising->addServiceUUID(cpsUUID);
    advertising->addServiceUUID(cscUUID);
    advertising->addServiceUUID(bsUUID);
    //advertising->setScanResponse(false);
    //advertising->setMinPreferred(0x0);
    server->start();
}

void BLE::loop() {
    const ulong t = millis();
    if (lastNotificationSent < t - 1000) {
        power = (short)board.getPower();
        // notify changed values
        if (connected) {
            bufPower[0] = powerFlags & 0xff;
            bufPower[1] = (powerFlags >> 8) & 0xff;
            bufPower[2] = power & 0xff;
            bufPower[3] = (power >> 8) & 0xff;
            cpmChar->setValue((uint8_t *)&bufPower, 4);
            cpmChar->notify();

            if (board.mpu.crankEventReady) {
                board.mpu.crankEventReady = false;
                uint16_t revolutions = board.mpu.revolutions;
                lastCrankEventTime += (uint16_t)(board.mpu.lastCrankEventTimeDiff * 1.024);
                bufCadence[0] = speedCadenceFlags & 0xff;
                bufCadence[1] = revolutions & 0xff;
                bufCadence[2] = (revolutions >> 8) & 0xff;
                bufCadence[3] = lastCrankEventTime & 0xff;
                bufCadence[4] = (lastCrankEventTime >> 8) & 0xff;
                cscmChar->setValue((uint8_t *)&bufCadence, 5);
                cscmChar->notify();
            }

            if (batteryLevel != oldBatteryLevel) {
                blChar->setValue(&batteryLevel, 1);
                blChar->notify();
                oldBatteryLevel = batteryLevel;
            }

            lastNotificationSent = t;
        }
    }
    if (!connected && oldConnected) {
        Serial.println("[BLE] Client disconnecting");
        oldConnected = connected;
    }
    if (connected && !oldConnected) {
        Serial.println("[BLE] Client connecting");
        oldConnected = connected;
    }
    if (!server->getAdvertising()->isAdvertising())
        startAdvertising();
}

void BLE::onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) {
    connected = true;
    Serial.println("[BLE] Server onConnect");
    //NimBLEDevice::startSecurity(desc->conn_handle);
}

void BLE::onDisconnect(BLEServer *pServer) {
    connected = false;
    Serial.println("[BLE] Server onDisconnect");
}

void BLE::startAdvertising() {
    delay(300);
    Serial.println("[BLE] Start advertising");
    server->startAdvertising();
}

void BLE::onRead(BLECharacteristic *pCharacteristic) {
    Serial.print(pCharacteristic->getUUID().toString().c_str());
    Serial.print(": onRead(), value: ");
    Serial.println(pCharacteristic->getValue().c_str());
};

void BLE::onWrite(BLECharacteristic *pCharacteristic) {
    Serial.print(pCharacteristic->getUUID().toString().c_str());
    Serial.print(": onWrite(), value: ");
    Serial.println(pCharacteristic->getValue().c_str());
};

void BLE::onNotify(BLECharacteristic *pCharacteristic){
    //Serial.printf("Sending notification: %d\n", pCharacteristic->getValue<int>());
};

void BLE::onSubscribe(BLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue) {
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
    Serial.println(pCharacteristic->getUUID().toString().c_str());
};

//bool BLE::onConfirmPIN(uint32_t pin) { Serial.println("onConfirmPIN"); return true; }
//uint32_t BLE::onPassKeyRequest() {Serial.println("onPassKeyRequest");  return 0; }
//void BLE::onAuthenticationComplete(ble_gap_conn_desc *desc) { Serial.println("onAuthenticationComplete"); }
