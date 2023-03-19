#if !defined(__mpu_temperature_h) && defined(FEATURE_MPU_TEMPERATURE)
#define __mpu_temperature_h

#ifndef FEATURE_MPU
#error FEATURE_MPU_TEMPERATURE requires FEATURE_MPU
#endif

#include "atoll_temperature_sensor.h"

class MpuTemperature : public Atoll::TemperatureSensor {
   public:
    typedef std::function<void(MpuTemperature *)> Callback;

    MpuTemperature(const char *label,
                   float updateFrequency = 1.0f,
                   Callback onValueChange = nullptr);

    virtual bool update();

    virtual void callOnValueChange() override;
    Callback onValueChange = nullptr;
};

#endif