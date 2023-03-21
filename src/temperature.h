#if !defined(__temperature_h) && defined(FEATURE_TEMPERATURE)
#define __temperature_h

#include "definitions.h"
#include "atoll_preferences.h"
#include "api.h"

#if !defined(FEATURE_DS18B20) && !defined(FEATURE_MPU_TEMPERATURE)
#error missing sensor support
#endif

#if defined(FEATURE_DS18B20) && defined(FEATURE_MPU_TEMPERATURE)
#error only one sensor is supported
#endif

#ifdef FEATURE_DS18B20
#include "atoll_ds18b20.h"
#endif  // FEATURE_DS18B20

#ifdef FEATURE_MPU_TEMPERATURE
#include "mpu_temperature.h"
#endif  // FEATURE_MPU_TEMPERATURE

#ifdef FEATURE_TEMPERATURE_COMPENSATION
#include "temperature_compensation.h"
#endif

class Temperature {
   public:
    typedef Atoll::TemperatureSensor Sensor;

#ifdef FEATURE_DS18B20
    typedef Atoll::DS18B20 DS18B20;
    DS18B20 *crankSensor = nullptr;
#else  // FEATURE_MPU_TEMPERATURE
    MpuTemperature *crankSensor = nullptr;
#endif

    Temperature();
    ~Temperature();

    void begin();

    void onCrankTemperatureChange(
#ifdef FEATURE_DS18B20
        DS18B20
#else  // FEATURE_MPU_TEMPERATURE
        MpuTemperature
#endif
            *sensor);

    void addApiCommand();
    Api::Result *tempProcessor(Api::Message *msg);

#ifdef FEATURE_TEMPERATURE_COMPENSATION
   public:
    typedef TemperatureCompensation TC;
    void setup(::Preferences *p, TC *tc);
    TC *tc = nullptr;
    bool setCompensationOffset();
    float getCompensation();

   protected:
    void setCompensation(float temperature);
    float compensationOffset = 0.0f;  // kg
    float compensation = 0.0f;        // kg
#else
   public:
    void setup(::Preferences *p);
#endif  // FEATURE_TEMPERATURE_COMPENSATION
};

#endif