#include <Arduino.h>
#include "mpu.h"
#include "ble.h"

MPU mpu;
BLE ble;

void setup() {
    Serial.begin(115200);
    mpu.setup();
    ble.setup();
}

void loop() {
    if (ble.connected) {
        mpu.loop();
        ble.power = (short)mpu.power;
        ble.revolutions = 2; // TODO
        ble.timestamp = (ushort)millis(); // TODO
    }
    ble.loop();
}