#include <Arduino.h>
#include <Preferences.h>

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
#include "power.h"

//#define LED_PIN 22

#define MPU_SDA_PIN 23
#define MPU_SCL_PIN 19

#define STRAIN_DOUT_PIN 5
#define STRAIN_SCK_PIN 18

#ifdef FEATURE_SERIALIO
SerialIO sio;
#endif
Preferences preferences;
Strain strain;
MPU mpu;
Power power;
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
    setCpuFrequencyMhz(160);
    esp_log_level_set("*", ESP_LOG_ERROR);
#ifdef FEATURE_SERIALIO
    sio.setup(&battery, &mpu, &strain, &power, &wifi);
#endif
    battery.setup(&preferences);
    strain.setup(STRAIN_DOUT_PIN, STRAIN_SCK_PIN, &preferences);
    mpu.setup(MPU_SDA_PIN, MPU_SCL_PIN, &preferences);
    power.setup(&mpu, &strain, &preferences);
    ble.setup();
    wifi.setup(&preferences);
#ifdef FEATURE_WEBSERVER
    ws.setup(&strain, &mpu, &power);
#endif
#ifdef FEATURE_OTA
    ota.setup();
#endif

    battery.taskStart("Battery Task", 1);
    strain.taskStart("Strain Task", 80);  // HX711 runs at max 80Hz
    mpu.taskStart("MPU Task", 80);        // no need to run faster than strain
    power.taskStart("Power Task", 80);
#ifdef FEATURE_SERIALIO
    sio.taskStart("SerialIO Task", 10);
#endif
#ifdef FEATURE_WEBSERVER
    ws.taskStart("Webserver Task", 20);
#endif
}

void loop() {
    const ulong t = millis();
    ble.loop(t);
    ble.batteryLevel = battery.level;
    wifi.loop(t);
#ifdef FEATURE_OTA
    ota.loop(t);
#endif
}
