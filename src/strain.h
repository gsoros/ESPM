#ifndef STRAIN_H
#define STRAIN_H

#include <Arduino.h>
#include <HX711_ADC.h>
#include <Preferences.h>
#include <driver/rtc_io.h>

#include "haspreferences.h"
#include "idle.h"
#include "task.h"

class Strain : public Idle, public Task, public HasPreferences {
   public:
    HX711_ADC *device;
    ulong measurementTime = 0;
    ulong lastMeasurementDelay = 0;
    gpio_num_t doutPin;
    gpio_num_t sckPin;

    void
    setup(const gpio_num_t doutPin,
          const gpio_num_t sckPin,
          Preferences *p,
          const char *preferencesNS = "STRAIN") {
        this->doutPin = doutPin;
        this->sckPin = sckPin;
        preferencesSetup(p, preferencesNS);
        rtc_gpio_hold_dis(sckPin);
        device = new HX711_ADC(doutPin, sckPin);
        device->begin();
        ulong stabilizingTime = 1000 / 80;
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

    void sleep() {
        device->powerDown();
        rtc_gpio_hold_en(sckPin);
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
        if (!preferencesStartLoad()) return;
        if (!preferences->getBool("calibrated", false)) {
            preferencesEnd();
            log_e("Device has not yet been calibrated");
            return;
        }
        device->setCalFactor(preferences->getFloat("calibration", 0));
        preferencesEnd();
    }

    void saveCalibration() {
        if (!preferencesStartSave()) return;
        preferences->putBool("calibrated", true);
        preferences->putFloat("calibration", device->getCalFactor());
        preferencesEnd();
    }

    void tare() {
        device->tare();
    }

   private:
    float _measurement = 0.0;
    bool _dataReady = false;
};

#endif