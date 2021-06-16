#include "ble.h"
#include "mpu.h"
#include "strain.h"
#include <Arduino.h>
#include <Preferences.h>

Preferences preferences;
Strain strain;
MPU mpu;
//BLE ble;

#ifdef STRAIN_USE_INTERRUPT
void strainDataReadyISR()
{
    if (strain.device->update()) {
        strain.dataReady = true;
    }
}
#endif

#ifdef MPU_USE_INTERRUPT
void mpuDataReadyISR()
{
    if (mpu.device->update()) {
        mpu.dataReady = true;
    }
}
#endif

char serialGetChar()
{
    while (!Serial.available()) {
        delay(10);
    }
    return Serial.read();
}

void serialOutput()
{
    static unsigned long lastSerialOutput = 0;
    unsigned long t = millis();
    if (lastSerialOutput < t - 1) {
        Serial.printf("%f %f %d %d\n",
                      //((int)t)%100,
                      mpu.yaw,
                      strain.lastMeasurement,
                      mpu.lastIdleCycles,
                      strain.lastIdleCycles);
        lastSerialOutput = t;
    }
}

void handleSerialInput()
{
    if (Serial.available() > 0) {
        switch (serialGetChar()) {
        case ' ':
            break;
        case 'c':
            mpu.calibrate();
            handleSerialInput();
            break;
        default:
            Serial.println("Press 'c' to calibrate, [space] to continue.");
            while (!Serial.available()) {
                delay(10);
            }
            handleSerialInput();
        }
    }
}

void setup()
{
    //Serial.println(getXtalFrequencyMhz());
    //while(1);
    setCpuFrequencyMhz(80);
    Serial.begin(115200);
    while (!Serial)
        delay(1);
    strain.setup();
#ifdef STRAIN_USE_INTERRUPT
    attachInterrupt(digitalPinToInterrupt(strain.doutPin), strainDataReadyISR, FALLING);
#endif
#ifdef MPU_USE_INTERRUPT
    attachInterrupt(digitalPinToInterrupt(mpu.intPin), mpuDataReadyISR, FALLING);
#endif
    mpu.setup(&preferences);
    //ble.setup();
    //while (1)
    //    ;
}
void loop()
{
    // if (ble.connected) {
    //     mpu.loop();
    //     ble.power = (short)mpu.power;
    //     ble.revolutions = 2; // TODO
    //     ble.timestamp = (ushort)millis(); // TODO
    // }
    // ble.loop();
    strain.loop();
    mpu.loop();
    serialOutput();
    handleSerialInput();
}
