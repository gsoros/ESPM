#ifndef BLE_H
#define BLE_H

#include "ble_constants.h"
#include <Arduino.h>
#include <NimBLEDevice.h>

class BLE : public BLEServerCallbacks
{
public:
    BLEServer *server;
    BLECharacteristic *cpmChar; // cycling power measurement char
    BLECharacteristic *blChar;  // battery level char
    uint8_t batteryLevel = 0;
    uint8_t oldBatteryLevel = 0;
    bool connected = false;
    bool oldConnected = false;
    unsigned long lastNotificationSent = 0;

    short power = 0;
    unsigned short revolutions = 0;
    unsigned short timestamp = 0;
    const unsigned short flags = 0x20; // TODO

    unsigned char bufMeasurent[8];
    unsigned char bufSensorLocation[1];
    unsigned char bufControlPoint[1];
    unsigned char bufFeature[4];

    void setup(const char *deviceName = "ESPM")
    {
        BLEDevice::init(deviceName);
        //NimBLEDevice::setSecurityAuth(false, false, false);
        //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        //NimBLEDevice::setSecurityPasskey(0);
        server = BLEDevice::createServer();
        server->setCallbacks(this);

        BLEUUID cpsUUID = BLEUUID(CYCLING_POWER_SERVICE_UUID);
        BLEService *cps = server->createService(cpsUUID);

        BLECharacteristic *cpfChar = cps->createCharacteristic(
            BLEUUID(CYCLING_POWER_FEATURE_CHAR_UUID),
            NIMBLE_PROPERTY::READ
            //| NIMBLE_PROPERTY::READ_ENC
        );
        bufFeature[0] = 0xff;
        bufFeature[1] = 0xff;
        bufFeature[2] = 0xff;
        bufFeature[3] = 0xff; // TODO
        cpfChar->setValue((uint8_t *)&bufFeature, 4);

        BLECharacteristic *slChar = cps->createCharacteristic(
            BLEUUID(SENSOR_LOCATION_CHAR_UUID),
            NIMBLE_PROPERTY::READ
            //| NIMBLE_PROPERTY::READ_ENC
        );
        bufSensorLocation[0] = SENSOR_LOCATION_RIGHT_CRANK & 0xff;
        slChar->setValue((uint8_t *)&bufSensorLocation, 1);

        /*
        BLECharacteristic *cpChar = cps->createCharacteristic(
            BLEUUID(SC_CONTROL_POINT_CHAR_UUID), 
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
        );
        bufControlPoint[0] = SC_CONTROL_POINT_OP_SET_CUMULATIVE_VALUE;
        cpChar->setValue((uint8_t *)&bufControlPoint, 1);
        */

        cpmChar = cps->createCharacteristic(
            BLEUUID(CYCLING_POWER_MEASUREMENT_CHAR_UUID),
            NIMBLE_PROPERTY::READ
                //| NIMBLE_PROPERTY::READ_ENC
                //| NIMBLE_PROPERTY::WRITE
                | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE);

        cps->start();

        BLEUUID bsUUID = BLEUUID(BATTERY_SERVICE_UUID);
        BLEService *bs = server->createService(bsUUID);

        blChar = bs->createCharacteristic(
            BLEUUID(BATTERY_LEVEL_CHAR_UUID),
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
        BLEDescriptor *blDesc = blChar->createDescriptor(BLEUUID(BATTERY_LEVEL_DESC_UUID));
        blDesc->setValue("Percentage");
        bs->start();

        BLEAdvertising *advertising = BLEDevice::getAdvertising();
        advertising->addServiceUUID(cpsUUID);
        advertising->addServiceUUID(bsUUID);
        //advertising->setScanResponse(false);
        //advertising->setMinPreferred(0x0);
        BLEDevice::startAdvertising();
    }

    void loop(const ulong t)
    {
        // notify changed value
        if (connected)
        {
            if (lastNotificationSent < t - 1000)
            {
                bufMeasurent[0] = flags & 0xff;
                bufMeasurent[1] = (flags >> 8) & 0xff;
                bufMeasurent[2] = power & 0xff;
                bufMeasurent[3] = (power >> 8) & 0xff;
                bufMeasurent[4] = revolutions & 0xff;
                bufMeasurent[5] = (revolutions >> 8) & 0xff;
                bufMeasurent[6] = timestamp & 0xff;
                bufMeasurent[7] = (timestamp >> 8) & 0xff;
                cpmChar->setValue((uint8_t *)&bufMeasurent, 8);
                cpmChar->notify();

                if (batteryLevel != oldBatteryLevel)
                {
                    blChar->setValue(&batteryLevel, 1);
                    blChar->notify();
                    oldBatteryLevel = batteryLevel;
                }
                lastNotificationSent = t;
            }
        }
        // disconnecting
        if (!connected && oldConnected)
        {
            delay(500);
            server->startAdvertising();
            log_i("start advertising");
            oldConnected = connected;
        }
        // connecting
        if (connected && !oldConnected)
        {
            oldConnected = connected;
        }
    }

    void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc)
    {
        connected = true;
        log_i("Server onConnect");
        //NimBLEDevice::startSecurity(desc->conn_handle);
    }

    void onDisconnect(BLEServer *pServer)
    {
        connected = false;
        log_i("Server onDisconnect");
    }

    //bool onConfirmPIN(uint32_t pin) { Serial.println("onConfirmPIN"); return true; }
    //uint32_t onPassKeyRequest() {Serial.println("onPassKeyRequest");  return 0; }
    //void onAuthenticationComplete(ble_gap_conn_desc *desc) { Serial.println("onAuthenticationComplete"); }
};

#endif