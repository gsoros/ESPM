#include "board.h"
#include "strain.h"

void Strain::setup(const gpio_num_t doutPin,
                   const gpio_num_t sckPin,
                   ::Preferences *p,
                   const char *preferencesNS) {
    this->doutPin = doutPin;
    this->sckPin = sckPin;
    _measurementBuf.clear();
    preferencesSetup(p, preferencesNS);
    rtc_gpio_hold_dis(sckPin);  // required to put HX711 into sleep mode
    device = new HX711_ADC(doutPin, sckPin);
    device->begin();
    log_i("[STRAIN] Starting HX711, tare");
    ulong stabilizingTime = 1000 / 80;  // 80 sps
    device->start(stabilizingTime, false);
    // device->tare();
    device->tareNoDelay();
    // if (device->getTareTimeoutFlag()) {
    //     Serial.println("[Strain] HX711 tare timeout");
    // }
    setAutoTareDelayMs(AUTO_TARE_DELAY_MS, false);
    loadSettings();
}

void Strain::loop() {
    if (1 != device->update())  // 1: data ready; 2: tare complete
        return;
    _measurementBuf.push(device->getData());
    ulong t = millis();
    if (board.motionDetectionMethod == MDM_STRAIN) {
        if (!_halfRevolution) {
            if (_measurementBuf.last() <= mdmStrainThresLow) {
                _halfRevolution = true;
            }
        } else if (mdmStrainThreshold <= _measurementBuf.last()) {
            _halfRevolution = false;
            board.motion.lastMovement = t;
            if (0 < board.motion.lastCrankEventTime) {
                ulong dt = t - board.motion.lastCrankEventTime;
                if (CRANK_EVENT_MIN_MS < dt) {
                    board.motion.revolutions++;
                    log_i("Crank event #%d dt: %ldms", board.motion.revolutions, dt);
                    board.power.onCrankEvent(dt);
                    board.bleServer.onCrankEvent(t, board.motion.revolutions);
                    board.motion.lastCrankEventTime = t;
                } else {
                    // Serial.printf("[STRAIN] Crank event skip, dt too small: %ldms\n", tDiff);
                }
            } else {
                board.motion.lastCrankEventTime = t;
            }
        }
    }
    if (autoTare && autoTareDelayMs < t) {
        ulong cutoff = t - autoTareDelayMs;
        if (board.motion.lastCrankEventTime < cutoff && _lastAutoTare < cutoff && 0 < _measurementBuf.size()) {
            float min = 1000.0;
            float max = -1000.0;
            int16_t firstSample = _measurementBuf.size() - autoTareSamples;
            if (firstSample < 0) firstSample = 0;
            for (decltype(_measurementBuf)::index_t i = _measurementBuf.size() - 1; firstSample < i; i--) {
                if (_measurementBuf[i] < min) min = _measurementBuf[i];
                if (max < _measurementBuf[i]) max = _measurementBuf[i];
            }
            _lastAutoTare = t;
            if (abs(max - min) < autoTareRangeG / 1000.0) {
                // log_i("Auto tare: %.2f, %.2f", min, max);
                device->tareNoDelay();
                //_lastAutoTare = t;
            } else {
                // log_i("Auto tare range too large: %fkg > %dg", max - min, autoTareRangeG);
            }
        }
    }
}

// returns the average of the measurement values in the buffer, optionally emptying the buffer
float Strain::value(bool clearBuffer) {
    float avg = 0.0;
    if (!dataReady()) return avg;
    float sum = 0.0;
    decltype(_measurementBuf)::index_t nValidMeasurements = 0;
    for (decltype(_measurementBuf)::index_t i = 0; i < _measurementBuf.size(); i++) {
        switch (negativeTorqueMethod) {
            case NTM_KEEP:
                nValidMeasurements++;
                sum += _measurementBuf[i];
                break;
            case NTM_ZERO:
                nValidMeasurements++;
                if (_measurementBuf[i] < 0.0)
                    sum += 0.0;
                else
                    sum += _measurementBuf[i];
                break;
            case NTM_DISCARD:
                if (0.0 <= _measurementBuf[i]) {
                    nValidMeasurements++;
                    sum += _measurementBuf[i];
                }
                break;
            case NTM_ABS:
                nValidMeasurements++;
                sum += abs(_measurementBuf[i]);
                break;
            default:  // invalid negativeTorqueMethod
                break;
        }
    }
    if (0 < nValidMeasurements)
        avg = sum / nValidMeasurements;
    if (clearBuffer) _measurementBuf.clear();

#if defined(FEATURE_TEMPERATURE) && defined(FEATURE_TEMPERATURE_COMPENSATION)
    avg += board.temperature.getCompensation();
#endif

    return avg;
}

float Strain::liveValue() {
    return dataReady() ? _measurementBuf.last()

#if defined(FEATURE_TEMPERATURE) && defined(FEATURE_TEMPERATURE_COMPENSATION)
                             + board.temperature.getCompensation()
#endif

                       : 0.0;
}

bool Strain::dataReady() {
    return 0 < _measurementBuf.size();
}

void Strain::sleep() {
    device->powerDown();
    rtc_gpio_hold_en(sckPin);
}

void Strain::setMdmStrainThreshold(int threshold) {
    mdmStrainThreshold = threshold;
}

void Strain::setMdmStrainThresLow(int threshold) {
    mdmStrainThresLow = threshold;
}

// calibrate to a known mass in kg
int Strain::calibrateTo(float knownMass) {
    float calFactor = device->getCalFactor();
    if (isnan(calFactor) ||
        isinf(calFactor) ||
        (-0.000000001 < calFactor && calFactor < 0.000000001)) {
        device->setCalFactor(1.0);
        delay(10);
    }
    if (isnan(knownMass) ||
        isinf(knownMass) ||
        (-0.000000001 < knownMass && knownMass < 0.000000001)) {
        log_e("Error setting calibraton factor.");
        return -1;
    }
    device->getNewCalibration(knownMass);
    return 0;
}

void Strain::printSettings() {
    log_i("Calibration factor: %f", device->getCalFactor());
}

void Strain::loadSettings() {
    if (!preferencesStartLoad()) return;
    mdmStrainThreshold = preferences->getInt("mdmSThres", mdmStrainThreshold);
    mdmStrainThresLow = preferences->getInt("mdmSThresL", mdmStrainThresLow);
    negativeTorqueMethod = (uint8_t)preferences->getUInt("negTorqMeth", negativeTorqueMethod);
    setAutoTare(preferences->getBool("autoTare", autoTare));
    setAutoTareDelayMs(preferences->getULong("ATDelayMs", autoTareDelayMs));
    setAutoTareRangeG(preferences->getUShort("ATRangeG", autoTareRangeG));
    if (!preferences->getBool("calibrated", false)) {
        preferencesEnd();
        log_e("Device has not yet been calibrated");
        return;
    }
    device->setCalFactor(preferences->getFloat("calibration", 0.0));
    preferencesEnd();
}

void Strain::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putInt("mdmSThres", mdmStrainThreshold);
    preferences->putInt("mdmSThresL", mdmStrainThresLow);
    preferences->putUInt("negTorqMeth", (uint32_t)negativeTorqueMethod);
    preferences->putBool("autoTare", autoTare);
    preferences->putULong("ATDelayMs", autoTareDelayMs);
    preferences->putUShort("ATRangeG", autoTareRangeG);
    preferences->putBool("calibrated", true);
    preferences->putFloat("calibration", device->getCalFactor());
    preferencesEnd();
}

void Strain::tare() {
    device->tare();
#if defined(FEATURE_TEMPERATURE) && defined(FEATURE_TEMPERATURE_COMPENSATION)
    board.temperature.setCompensationOffset();
#endif
}

bool Strain::getAutoTare() {
    return autoTare;
}

void Strain::setAutoTare(bool val) {
    autoTare = val;
    log_i("autoTare=%d", autoTare);
}

ulong Strain::getAutoTareDelayMs() {
    return autoTareDelayMs;
}

void Strain::setAutoTareDelayMs(ulong val, bool log) {
    autoTareDelayMs = val;
    autoTareSamples = val / 1000 * STRAIN_TASK_FREQ;
    if (log) log_i("autoTareDelayMs=%lu autoTareSamples=%d", autoTareDelayMs, autoTareSamples);
}

uint16_t Strain::getAutoTareRangeG() {
    return autoTareRangeG;
}

void Strain::setAutoTareRangeG(uint16_t val) {
    autoTareRangeG = val;
    log_i("autoTareRangeG=%d", autoTareRangeG);
}