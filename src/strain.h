#ifndef STRAIN_H
#define STRAIN_H

#include "idle.h"
#include <Arduino.h>
#include <HX711_ADC.h>

#define STRAIN_DOUT_PIN 5
#define STRAIN_SCK_PIN 18
#define STRAIN_USE_INTERRUPT

class Strain : public Idle
{
public:
    HX711_ADC *device;
    const uint8_t doutPin = STRAIN_DOUT_PIN;
    const uint8_t sckPin = STRAIN_SCK_PIN;
#ifdef STRAIN_USE_INTERRUPT
    volatile bool dataReady;
#endif
    float lastMeasurement = 0;
    unsigned long lastMeasurementTime = 0;

    void setup()
    {
        device = new HX711_ADC(doutPin, sckPin);
        device->begin();
        float calibrationValue = 696.0;
        unsigned long stabilizingTime = 2000;
        bool tare = true;
        device->start(stabilizingTime, tare);
        if (device->getTareTimeoutFlag()) {
            Serial.println("[Error] HX711 Tare Timeout");
            while (1)
                ;
        }
        device->setCalFactor(calibrationValue);
    }

    void loop()
    {
#ifdef STRAIN_USE_INTERRUPT
        if (dataReady) {
#endif
            if (device->update()) {
                lastMeasurement = device->getData();
                lastMeasurementTime = millis();
                resetIdleCycles();
#ifdef STRAIN_USE_INTERRUPT
                dataReady = false;
#endif
                return;
            }
#ifdef STRAIN_USE_INTERRUPT
        }
#endif
        increaseIdleCycles();
    }
};

#endif