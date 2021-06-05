#ifndef BLE_H
#define BLE_H

#include <NimBLEDevice.h>
#include "ble_constants.h"

class BLE : public BLEServerCallbacks {
    public:
    BLEServer *server;
    BLECharacteristic *measurementCharacteristic;
    bool connected = false;
    bool oldConnected = false;
    
    short power = 0;
    unsigned short revolutions = 0;
    unsigned short timestamp = 0;
    const unsigned short flags = 0x20; // TODO

    unsigned char bufMeasurent[8];
    unsigned char bufSensorLocation[1];
    unsigned char bufControlPoint[1];
    unsigned char bufFeature[4];

    void setup() {
        BLEDevice::init("PM");
        //NimBLEDevice::setSecurityAuth(false, false, false);
        //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        //NimBLEDevice::setSecurityPasskey(0);
        this->server = BLEDevice::createServer();
        this->server->setCallbacks(this);
        BLEUUID serviceUUID = BLEUUID(CYCLING_POWER_SERVICE_UUID);
        BLEService *service = this->server->createService(serviceUUID);

        BLECharacteristic *featureCharasteristic = service->createCharacteristic(
            BLEUUID(CYCLING_POWER_FEATURE_CHAR_UUID), 
            NIMBLE_PROPERTY::READ
            //| NIMBLE_PROPERTY::READ_ENC
        );
        this->bufFeature[0] = 0xee;
        this->bufFeature[1] = 0xee;
        this->bufFeature[2] = 0xee;
        this->bufFeature[3] = 0xee; // TODO
        featureCharasteristic->setValue((uint8_t *)&this->bufFeature, 4);
        
        BLECharacteristic *sensorLocationCharasteristic = service->createCharacteristic(
            BLEUUID(SENSOR_LOCATION_CHAR_UUID), 
            NIMBLE_PROPERTY::READ
            //| NIMBLE_PROPERTY::READ_ENC
        );
        this->bufSensorLocation[0] = SENSOR_LOCATION_RIGHT_CRANK & 0xff;
        sensorLocationCharasteristic->setValue((uint8_t *)&this->bufSensorLocation, 1);

        /*
        BLECharacteristic *controlPointCharasteristic = service->createCharacteristic(
            BLEUUID(SC_CONTROL_POINT_CHAR_UUID), 
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
        );
        this->bufControlPoint[0] = SC_CONTROL_POINT_OP_SET_CUMULATIVE_VALUE;
        controlPointCharasteristic->setValue((uint8_t *)&this->bufControlPoint, 1);
        */

        this->measurementCharacteristic = service->createCharacteristic(
            BLEUUID(CYCLING_POWER_MEASUREMENT_CHAR_UUID),
            NIMBLE_PROPERTY::READ 
            //| NIMBLE_PROPERTY::READ_ENC
            //| NIMBLE_PROPERTY::WRITE
            | NIMBLE_PROPERTY::NOTIFY
            | NIMBLE_PROPERTY::INDICATE
        );
        
        service->start();
        BLEAdvertising *advertising = BLEDevice::getAdvertising();
        advertising->addServiceUUID(serviceUUID);
        //advertising->setScanResponse(false);
        //advertising->setMinPreferred(0x0);
        BLEDevice::startAdvertising();
    }

    void loop() {
        // notify changed value
        if (this->connected) {
            this->bufMeasurent[0] = this->flags & 0xff;
            this->bufMeasurent[1] = (this->flags >> 8) & 0xff;
            this->bufMeasurent[2] = this->power & 0xff;
            this->bufMeasurent[3] = (this->power >> 8) & 0xff;
            this->bufMeasurent[4] = this->revolutions & 0xff;
            this->bufMeasurent[5] = (this->revolutions >> 8) & 0xff;
            this->bufMeasurent[6] = this->timestamp & 0xff;
            this->bufMeasurent[7] = (this->timestamp >> 8) & 0xff;

            this->measurementCharacteristic->setValue((uint8_t *)&this->bufMeasurent, 8);
            this->measurementCharacteristic->notify();
            delay(1000); 
        }
        // disconnecting
        if (!this->connected && this->oldConnected) {
            delay(500); 
            this->server->startAdvertising();
            Serial.println("start advertising");
            this->oldConnected = this->connected;
        }
        // connecting
        if (this->connected && !this->oldConnected) {
            this->oldConnected = this->connected;
        }
    }

    void onConnect(BLEServer *pServer, ble_gap_conn_desc* desc) {
        this->connected = true;
        Serial.println("Server onConnect");
        //NimBLEDevice::startSecurity(desc->conn_handle); 
    }

    void onDisconnect(BLEServer *pServer) {
        this->connected = false;
        Serial.println("Server onDisconnect");
    }

    //bool onConfirmPIN(uint32_t pin) { Serial.println("onConfirmPIN"); return true; }
    //uint32_t onPassKeyRequest() {Serial.println("onPassKeyRequest");  return 0; }
    //void onAuthenticationComplete(ble_gap_conn_desc *desc) { Serial.println("onAuthenticationComplete"); }
};

#endif