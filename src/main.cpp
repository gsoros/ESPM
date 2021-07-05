#define NO_GLOBAL_SERIAL  // silence intellisense
#define MPU_RINGBUF_SIZE 16
#define WIFISERIAL_RINGBUF_RX_SIZE 256
#define WIFISERIAL_RINGBUF_TX_SIZE 1024
#define HOSTNAME "ESPM"
//#define LED_PIN 22
#define MPU_SDA_PIN 23
#define MPU_SCL_PIN 19
#define STRAIN_DOUT_PIN 5
#define STRAIN_SCK_PIN 18

#include <Arduino.h>
#include <Preferences.h>

#include "serialsplitter.h"
#include "battery.h"
#include "ble.h"
#include "mpu.h"
#include "status.h"
#include "strain.h"
#include "wificonnection.h"
#ifdef FEATURE_WEBSERVER
#include "webserver.h"
#endif
#ifdef FEATURE_OTA
#include "ota.h"
#endif
#include "power.h"
#include "status.h"
#include "wifiserial.h"

HardwareSerial hwSerial(0);
WifiSerial wifiSerial;
Preferences preferences;
Status status;
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
    //Serial.println(getXtalFrequencyMhz()); while(1);
    //setCpuFrequencyMhz(160);
    esp_log_level_set("*", ESP_LOG_DEBUG);
    hwSerial.begin(115200);
    wifi.setup(&preferences);
    while (!hwSerial)
        vTaskDelay(10);
    wifiSerial.setup();
    Serial.setup(&hwSerial, &wifiSerial, true, true);
    status.setup(&battery, &mpu, &strain, &power, &wifi);
    battery.setup(&preferences);
    strain.setup(STRAIN_DOUT_PIN, STRAIN_SCK_PIN, &preferences);
    mpu.setup(MPU_SDA_PIN, MPU_SCL_PIN, &preferences);
    power.setup(&mpu, &strain, &preferences);
    ble.setup();
#ifdef FEATURE_WEBSERVER
    ws.setup(&strain, &mpu, &power);
#endif
#ifdef FEATURE_OTA
    ota.setup(HOSTNAME);
#endif

    battery.taskStart("Battery Task", 1);
    strain.taskStart("Strain Task", 90);
    mpu.taskStart("MPU Task", 125);
    power.taskStart("Power Task", 90);
    wifiSerial.taskStart("WifiSerial Task", 10);
    status.taskStart("Status Task", 10);
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
