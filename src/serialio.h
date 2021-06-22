#ifndef SERIALIO_H
#define SERIALIO_H

#include "battery.h"
#include "mpu.h"
#include "strain.h"
#include <Arduino.h>

class SerialIO
{
public:
    Battery *battery;
    MPU *mpu;
    Strain *strain;
    bool statusEnabled = false;

    void setup(Battery *b, MPU *m, Strain *s)
    {
        battery = b;
        mpu = m;
        strain = s;
        Serial.begin(115200);
        while (!Serial)
            delay(1);
        statusEnabled = true;
    }

    void loop(const ulong t)
    {
        if (Serial.available() > 0) {
            handleInput();
        }
        printStatus(t);
    }

    char getChar()
    {
        while (!Serial.available()) {
            delay(10);
        }
        return Serial.read();
    }

    void printStatus(const ulong t)
    {
        static ulong lastOutput = 0;
        if (!statusEnabled)
            return;
        if (lastOutput < t - 2000) {
            Serial.printf(
                //"%f %f %d %d\n",
                "%f %f %f\n",
                //((int)t)%100,
                mpu->measurement,
                strain->measurement,
                battery->voltage
                //mpu->idleCyclesMax,
                //strain->idleCyclesMax
            );
            //mpu->idleCyclesMax = 0;
            //strain->idleCyclesMax = 0;
            lastOutput = t;
        }
    }

    void handleInput(const char input = '\0')
    {
        switch ('\0' == input ? getChar() : input) {
        case 'x':
            break;
        case 's':
            statusEnabled = !statusEnabled;
            if (!statusEnabled)
                Serial.println("Status output paused, press [s] to resume");
            break;
        case 'c':
            mpu->calibrate();
            //handleInput('h');
            break;
        case 'h':
            Serial.println("[c]alibrate, [w]ifi, [s]tatus, [r]eboot");
            break;
        }
    }
};

#endif