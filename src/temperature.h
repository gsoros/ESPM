#if !defined(__temperature_h) && defined(FEATURE_TEMPERATURE)
#define __temperature_h

#include "definitions.h"
#include "atoll_temperature_sensor.h"
#ifdef FEATURE_TEMPERATURE_COMPENSATION
#include "temperature_compensation.h"
#endif

class Temperature {
   public:
    typedef Atoll::TemperatureSensor Sensor;

    Sensor *crankSensor = nullptr;

    Temperature();
    ~Temperature();

    void begin();

    void onSensorValueChange(Sensor *sensor);

#ifdef FEATURE_TEMPERATURE_COMPENSATION
    typedef TemperatureCompensation TC;
    void setup(TC *tc);
    TC *tc = nullptr;
    bool setCompensationOffset();
    float getCompensation();

   protected:
    void setCompensation(float temperature);
    float compensationOffset = 0.0f;  // kg
    float compensation = 0.0f;        // kg
#else
    void setup();
#endif  // FEATURE_TEMPERATURE_COMPENSATION
};

#endif