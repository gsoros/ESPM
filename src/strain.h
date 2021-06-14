#ifndef STRAIN_H
#define STRAIN_H

#include <HX711_ADC.h>
#include "idle.h"

#define DOUT_PIN 5
#define SCK_PIN 18

class Strain : public Idle {
    public:
    HX711_ADC *adc;
    float lastMeasurement = 0;
    unsigned long lastMeasurementTime = 0;

    void setup() {
        adc = new HX711_ADC(DOUT_PIN, SCK_PIN);
        adc->begin();
        float calibrationValue = 696.0; 
        unsigned long stabilizingTime = 2000;
        bool tare = true;
        adc->start(stabilizingTime, tare);
        if (adc->getTareTimeoutFlag()) {
            Serial.println("[Error] HX711 Tare Timeout");
            while (1);
        }
        else {
            adc->setCalFactor(calibrationValue);
        }
    }

    void loop() {
        if (adc->update()) {
            lastMeasurement = adc->getData();
            lastMeasurementTime = millis();
            resetIdleCycles();
        }
        else {
            increaseIdleCycles();
        }
    }
};

#endif