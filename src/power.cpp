#include "power.h"
#include "board.h"
#include "mpu.h"
#include "strain.h"

void Power::setup(Preferences *p) {
    preferencesSetup(p, "POWER");
    loadSettings();
}

void Power::loop(const ulong t) {
    static ulong previousT = t;
    if (t == previousT) return;
    if (!board.strain.dataReady()) {
        //log_e("strain not ready, skipping loop at %d, SPS=%f", t, board.strain.device->getSPS());
        return;
    }

    double deltaT = (t - previousT) / 1000.0;                                               // t(s)
    float rpm = filterNegative(board.mpu.rpm(), reverseMPU);                                //
    float force = filterNegative(board.strain.measurement(true), reverseStrain) * 9.80665;  // F(N)   = m(Kg) * G(m/s/s)
    float diameter = 2.0 * crankLength * PI / 1000.0;                                       // d(m)   = 2 * r(mm) * π / 1000
    double distance = diameter * rpm / 60.0 * deltaT;                                       // s(m)   = d(m) * rev/s * t(s)
    double velocity = distance / deltaT;                                                    // v(m/s) = s(m) / t(s)
    double work = force * velocity;                                                         // W(J)   = F(N) * v(m)
    double power = work / deltaT;                                                           // P(W)   = W(J) / t(s)
                                                                                            // P      = F d RPM / 60 / t

    /*
        _power = filterNegative(board.strain.measurement(true), reverseStrain) * 9.80665 *
                 2.0 * crankLength * PI / 1000.0 *
                 filterNegative(board.mpu.rpm(), reverseMPU) / 60.0 /
                 (t - previousT) / 1000.0;
        */

    /*
        _power = filterNegative(board.strain.measurement(true), reverseStrain) *
                 filterNegative(board.mpu.rpm(), reverseMPU) *
                 crankLength / 
                 (t - previousT) *
                 0.000001026949987;
        */

    //if (_powerBuf.isFull()) log_e("power buffer is full");
    _powerBuf.push((float)power);
    previousT = t;
}

// Returns the average of the buffered power values, not emptying the buffer by default.
// Measurements are added to the buffer at the speed of taskFreq.
// To avoid data loss, POWER_RINGBUF_SIZE should be large enough to hold the
// measurements between calls.
float Power::power() { return power(false); }
float Power::power(bool clearBuffer) {
    float power = 0;
    if (_powerBuf.isEmpty()) return power;
    for (decltype(_powerBuf)::index_t i = 0; i < _powerBuf.size(); i++) {
        power += _powerBuf[i] / _powerBuf.size();
    }
    if (clearBuffer) _powerBuf.clear();
    return power;
}

void Power::loadSettings() {
    if (!preferencesStartLoad()) return;
    crankLength = preferences->getFloat("crankLength", 172.5);
    reverseMPU = preferences->getBool("reverseMPU", false);
    reverseStrain = preferences->getBool("reverseStrain", false);
    preferencesEnd();
}

void Power::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putFloat("crankLength", crankLength);
    preferences->putBool("reverseMPU", reverseMPU);
    preferences->putBool("reverseStrain", reverseStrain);
    preferencesEnd();
}

void Power::printSettings() {
    Serial.printf("Crank length: %.2fmm\nStrain is %sreversed\nMPU is %sreversed\n",
                  crankLength, reverseStrain ? "" : "not ", reverseMPU ? "" : "not ");
}

float Power::filterNegative(float value) { return filterNegative(value, false); }
float Power::filterNegative(float value, bool reverse) {
    if (reverse) value *= -1;
    if (value < 0.0)
        return 0.0;
    else
        return value;
}
