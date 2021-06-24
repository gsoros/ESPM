#ifndef STRAIN_H
#define STRAIN_H

#include "idle.h"
#include <Arduino.h>
#include <HX711_ADC.h>

#define STRAIN_DOUT_PIN 5
#define STRAIN_SCK_PIN 18
//#define STRAIN_USE_INTERRUPT

#ifdef STRAIN_USE_INTERRUPT
volatile bool strainDataReady;
void IRAM_ATTR strainDataReadyISR()
{
    strainDataReady = true;
}
#endif

class Strain : public Idle
{
public:
    HX711_ADC *device;
    const uint8_t doutPin = STRAIN_DOUT_PIN;
    const uint8_t sckPin = STRAIN_SCK_PIN;

    float measurement = 0.0;
    ulong measurementTime = 0;

    void setup()
    {
        device = new HX711_ADC(doutPin, sckPin);
        device->begin();
        float calibrationValue = 696.0;
        ulong stabilizingTime = 2000;
        bool tare = true;
        device->start(stabilizingTime, tare);
        if (device->getTareTimeoutFlag())
        {
            log_e("[Error] HX711 Tare Timeout");
        }
        device->setCalFactor(calibrationValue);
#ifdef STRAIN_USE_INTERRUPT
        attachInterrupt(digitalPinToInterrupt(doutPin), strainDataReadyISR, FALLING);
#endif
    }

    void loop(const ulong t)
    {
#ifdef STRAIN_USE_INTERRUPT
        if (strainDataReady)
        {
#endif
            if (device->update())
            {
                measurement = device->getData();
                measurementTime = t;
                resetIdleCycles();
#ifdef STRAIN_USE_INTERRUPT
                strainDataReady = false;
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