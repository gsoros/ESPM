#ifndef STRAIN_H
#define STRAIN_H

#include <Arduino.h>
#include <HX711_ADC.h>
#include <Preferences.h>

#include "idle.h"

#define STRAIN_DOUT_PIN 5
#define STRAIN_SCK_PIN 18
//#define STRAIN_USE_INTERRUPT

#ifdef STRAIN_USE_INTERRUPT
volatile bool strainDataReady;
void IRAM_ATTR strainDataReadyISR() {
    strainDataReady = true;
}
#endif

class Strain : public Idle {
   public:
    HX711_ADC *device;
    const uint8_t doutPin = STRAIN_DOUT_PIN;
    const uint8_t sckPin = STRAIN_SCK_PIN;
    Preferences *preferences;
    const char *preferencesNS;

    float measurement = 0.0;
    ulong measurementTime = 0;

    void setup(Preferences *p,
               const char *preferencesNS = "STRAIN") {
        preferences = p;
        this->preferencesNS = preferencesNS;
        device = new HX711_ADC(doutPin, sckPin);
        device->begin();
        ulong stabilizingTime = 2000;
        bool tare = true;
        device->start(stabilizingTime, tare);
        if (device->getTareTimeoutFlag()) {
            log_e("[Error] HX711 Tare Timeout");
        }
        loadCalibration();
#ifdef STRAIN_USE_INTERRUPT
        attachInterrupt(digitalPinToInterrupt(doutPin), strainDataReadyISR, FALLING);
#endif
    }

    void loop(const ulong t) {
#ifdef STRAIN_USE_INTERRUPT
        if (strainDataReady) {
#endif
            if (device->update()) {
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

    void calibrateTo(float knownMass) {
        float calFactor = device->getCalFactor();
        if (isnan(calFactor) ||
            isinf(calFactor) ||
            (-0.000000001 < calFactor && calFactor < 0.000000001) ||
            isnan(knownMass) ||
            isinf(knownMass) ||
            (-0.000000001 < knownMass && knownMass < 0.000000001)) {
            device->setCalFactor(1.0);
            return;
        }
        device->getNewCalibration(knownMass);
    }

    void printCalibration() {
        Serial.printf("Strain calibration factor: %f\n", device->getCalFactor());
    }

    void loadCalibration() {
        if (!preferences->begin(preferencesNS, true))  // try ro mode
        {
            if (!preferences->begin(preferencesNS, false))  // open in rw mode to create ns
            {
                log_e("Preferences begin failed for '%s'\n", preferencesNS);
                return;
            }
        }
        if (!preferences->getBool("calibrated", false)) {
            preferences->end();
            log_e("Device has not yet been calibrated");
            return;
        }
        device->setCalFactor(preferences->getFloat("calibration", 0));
        preferences->end();
    }

    void saveCalibration() {
        if (!preferences->begin(preferencesNS, false)) {
            log_e("Preferences begin failed for '%s'.", preferencesNS);
            return;
        }
        preferences->putBool("calibrated", true);
        preferences->putFloat("calibration", device->getCalFactor());
        preferences->end();
    }

    void tare() {
        device->tare();
    }
};

#endif