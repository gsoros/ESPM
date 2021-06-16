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
void IRAM_ATTR strainDataReadyISR()
{
    strain.dataReady = true;
}
#endif

#ifdef MPU_USE_INTERRUPT
void IRAM_ATTR mpuDataReadyISR()
{
    mpu.dataReady = true;
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
    if (lastSerialOutput < t - 10) {
        Serial.printf("%f %f %d %d\n",
                      //((int)t)%100,
                      mpu.yaw,
                      strain.lastMeasurement,
                      mpu.idleCyclesMax,
                      strain.idleCyclesMax);
        mpu.idleCyclesMax = 0;
        strain.idleCyclesMax = 0;
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
    if (Serial.available() > 0) {
        handleSerialInput();
    }
}
