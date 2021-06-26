#include "battery.h"
#include "ble.h"
#include "mpu.h"
#include "serialio.h"
#include "strain.h"
#include "wificonnection.h"
#ifdef FETURE_WEBSERVER
#include "webserver.h"
#endif
#include <Arduino.h>
#include <Preferences.h>

//#define LED_PIN 22
#define MPU_SDA_PIN 23
#define MPU_SCL_PIN 19

SerialIO sio;
Preferences preferences;
Strain strain;
MPU mpu;
BLE ble;
Battery battery;
WifiConnection wifi;
#ifdef FETURE_WEBSERVER
WebServer ws;
#endif

void setup()
{
    //Serial.println(getXtalFrequencyMhz());
    //while(1);
    //setCpuFrequencyMhz(160);
    sio.setup(&battery, &mpu, &strain, &wifi);
    strain.setup();
    mpu.setup(MPU_SDA_PIN, MPU_SCL_PIN, &preferences);
    ble.setup();
    wifi.setup(&preferences);
#ifdef FETURE_WEBSERVER
    ws.setup(&mpu);
#endif
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
    mpu.loop(t);
#ifdef FETURE_WEBSERVER
    ws.strain = strain.measurement;
    ws.qX = mpu.qX;
    ws.qY = mpu.qY;
    ws.qZ = mpu.qZ;
    ws.qW = mpu.qW;
    ws.loop(t);
#endif
    sio.loop(t);
    ble.loop(t);
    battery.loop(t);
    ble.batteryLevel = battery.level;
    wifi.loop(t);
}
