#include "power.h"
#include "board.h"
#include "mpu.h"
#include "strain.h"

void Power::setup(Preferences *p) {
    preferencesSetup(p, "POWER");
    loadSettings();
}

void Power::loop() {
    const ulong t = millis();
    static ulong previousT = t;
    if (t == previousT) return;
    if (!board.strain.dataReady()) {
        //log_e("strain not ready, skipping loop at %d, SPS=%f", t, board.strain.device->getSPS());
        return;
    }

    double deltaT = (t - previousT) / 1000.0;                       // t(s)
    float rps = filterNegative(board.getRpm(), reverseMPU) / 60;    // revs per sec
    float mass = filterNegative(board.getStrain(), reverseStrain);  // m(kg)
    float force = mass * 9.80665;                                   // F(N)   = m(kg) * G(m/s/s)
    float radius = crankLength / 1000.0;                            // r(m)
    float circumference = 2.0 * radius * PI;                        // C(m)   = 2 * r(m) * π
    double distance = circumference * rps * deltaT;                 // s(m)   = C(m) * rps * t(s)
    double velocity = distance / deltaT;                            // v(m/s) = s(m) / t(s)
    double power = force * velocity;                                // P(W)   = F(N) * v(m/s)
    // P      = F * v
    // P      = m * G * s / t
    // P      = m * G * C * rps * t / t
    // P      = m * G * 2 * r * π * rps
    // P      = m * rps * r * G * π * 2
    if (power < 0.0)
        power = 0.0;
    else if (10000.0 < power)
        power = 10000.0;
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
    Serial.printf("[Power] Crank length: %.2fmm\nStrain is %sreversed\nMPU is %sreversed\n",
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
