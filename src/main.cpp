#include "battery.h"
#include "ble.h"
#include "mpu.h"
#ifdef FEATURE_SERIALIO
#include "serialio.h"
#endif
#include "strain.h"
#include "wificonnection.h"
#ifdef FEATURE_WEBSERVER
#include "webserver.h"
#endif
#ifdef FEATURE_OTA
#include "ota.h"
#endif
#include <Arduino.h>
#include <Preferences.h>

//#define LED_PIN 22
#define MPU_SDA_PIN 23
#define MPU_SCL_PIN 19

#ifdef FEATURE_SERIALIO
SerialIO sio;
#endif
Preferences preferences;
Strain strain;
MPU mpu;
BLE ble;
Battery battery;
WifiConnection wifi;
#ifdef FEATURE_WEBSERVER
WebServer ws;
#endif
#ifdef FEATURE_OTA
OTA ota;
#endif

void setup() {
    //Serial.println(getXtalFrequencyMhz());
    //while(1);
    //setCpuFrequencyMhz(160);
#ifdef FEATURE_SERIALIO
    sio.setup(&battery, &mpu, &strain, &wifi);
#endif
    battery.setup(&preferences);
    strain.setup();
    mpu.setup(MPU_SDA_PIN, MPU_SCL_PIN, &preferences);
    ble.setup();
    wifi.setup(&preferences);
#ifdef FEATURE_WEBSERVER
    ws.setup(&mpu);
#endif
#ifdef FEATURE_OTA
    ota.setup();
#endif
}

void loop() {
    const ulong t = millis();
    // if (ble.connected) {
    //     mpu.loop();
    //     ble.power = (short)mpu.power;
    //     ble.revolutions = 2; // TODO
    //     ble.timestamp = (ushort)millis(); // TODO
    // }
    strain.loop(t);
    mpu.loop(t);
#ifdef FEATURE_WEBSERVER
    ws.strain = strain.measurement;
    ws.qX = mpu.qX;
    ws.qY = mpu.qY;
    ws.qZ = mpu.qZ;
    ws.qW = mpu.qW;
    ws.loop(t);
#endif
#ifdef FEATURE_SERIALIO
    sio.loop(t);
#endif
    ble.loop(t);
    battery.loop(t);
    ble.batteryLevel = battery.level;
    wifi.loop(t);
#ifdef FEATURE_OTA
    ota.loop(t);
#endif
}
