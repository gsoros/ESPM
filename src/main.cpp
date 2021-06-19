#include "battery.h"
#include "ble.h"
#include "mpu.h"
#include "strain.h"
#include "websockets.h"
#include "wificonnection.h"
#include <Arduino.h>
#include <Preferences.h>

//#define LED_PIN 22

Preferences preferences;
Strain strain;
MPU mpu;
BLE ble;
Battery battery;
Websockets websockets;
WifiConnection wifi;

char serialGetChar()
{
    while (!Serial.available()) {
        delay(10);
    }
    return Serial.read();
}

void serialOutput(const ulong t)
{
    static ulong lastSerialOutput = 0;
    if (lastSerialOutput < t - 2000) {
        Serial.printf(
            //"%f %f %d %d\n",
            "%f %f %f\n",
            //((int)t)%100,
            mpu.measurement,
            strain.lastMeasurement,
            battery.voltage
            //mpu.idleCyclesMax,
            //strain.idleCyclesMax
        );
        //mpu.idleCyclesMax = 0;
        //strain.idleCyclesMax = 0;
        lastSerialOutput = t;
    }
}

void handleSerialInput()
{
    while (!Serial.available()) {
        delay(10);
    }
    switch (serialGetChar()) {
    case ' ':
        break;
    case 'p':
        Serial.println("Paused");
        handleSerialInput();
        break;
    case 'c':
        mpu.calibrate();
        Serial.println("Press 'c' to recalibrate, [space] to continue.");
        handleSerialInput();
        break;
    default:
        Serial.println("Press 'c' to calibrate, [space] to continue.");
        handleSerialInput();
    }
}

void setup()
{
    //Serial.println(getXtalFrequencyMhz());
    //while(1);
    setCpuFrequencyMhz(80);
    //esp_log_level_set();
    Serial.begin(115200);
    while (!Serial)
        delay(1);
    strain.setup();
    mpu.setup(&preferences);
    ble.setup();
    wifi.setup(&preferences, &mpu);
    websockets.setup();
}
void loop()
{
    const ulong t = millis();
    // if (ble.connected) {
    //     mpu.loop();
    //     ble.power = (short)mpu.power;
    //     ble.revolutions = 2; // TODO
    //     ble.timestamp = (ushort)millis(); // TODO
    // }
    strain.loop(t);
    websockets.strain = strain.lastMeasurement;
    mpu.loop(t);
    websockets.qX = mpu.qX;
    websockets.qY = mpu.qY;
    websockets.qZ = mpu.qZ;
    websockets.qW = mpu.qW;
    serialOutput(t);
    ble.loop(t);
    if (Serial.available() > 0) {
        handleSerialInput();
    }
    battery.loop(t);
    ble.batteryLevel = battery.level;
    wifi.loop(t);
    websockets.loop(t);
}
