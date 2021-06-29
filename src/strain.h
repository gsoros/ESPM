#ifndef STRAIN_H
#define STRAIN_H

#include <Arduino.h>
#include <HX711_ADC.h>
#include <Preferences.h>

#include "idle.h"
#include "task.h"

class Strain : public Idle, public Task {
   public:
    HX711_ADC *device;
    Preferences *preferences;
    const char *preferencesNS;

    ulong measurementTime = 0;
    ulong lastMeasurementDelay = 0;

    void setup(const uint8_t doutPin,
               const uint8_t sckPin,
               Preferences *p,
               const char *preferencesNS = "STRAIN") {
        preferences = p;
        this->preferencesNS = preferencesNS;
        device = new HX711_ADC(doutPin, sckPin);
        device->begin();
        ulong stabilizingTime = 2000;
        bool tare = true;
        device->start(stabilizingTime, tare);
        if (device->getTareTimeoutFlag()) {
            log_e("HX711 Tare Timeout");
        }
        loadCalibration();
    }

    void loop(const ulong t) {
        if (device->update()) {
            _measurement = device->getData();
            lastMeasurementDelay = t - measurementTime;
            measurementTime = t;
            _dataReady = true;
            return;
        }
        increaseIdleCycles();
    }

    float measurement(bool unsetDataReadyFlag = false) {
        if (unsetDataReadyFlag) _dataReady = false;
        return _measurement;
    }

    bool dataReady() {
        return _dataReady;
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

   private:
    float _measurement = 0.0;
    bool _dataReady = false;
};

#endif