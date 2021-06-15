#include <Arduino.h>
#include "strain.h"
#include "mpu.h"
//#include "ble.h"

Strain strain;
MPU mpu;
//BLE ble;

unsigned long lastSerialOutput = 0;

void setup() {
    Serial.begin(115200);
    //Serial.println(getXtalFrequencyMhz());
    //while(1);
    setCpuFrequencyMhz(80);
    strain.setup();
    mpu.setup();
    //ble.setup();
}

void loop() {
    // if (ble.connected) {
    //     mpu.loop();
    //     ble.power = (short)mpu.power;
    //     ble.revolutions = 2; // TODO
    //     ble.timestamp = (ushort)millis(); // TODO
    // }
    // ble.loop();
    strain.loop();
    mpu.loop();

    unsigned long t = millis();
    if (lastSerialOutput < t - 1) {
        Serial.printf("%f %f %d %d\n", 
            //((int)t)%100, 
            mpu.yaw, 
            strain.lastMeasurement,
            mpu.lastIdleCycles,
            strain.lastIdleCycles
        );
        lastSerialOutput = t;
    }
}