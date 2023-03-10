#if !defined(__temperature_h) && defined(FEATURE_TEMPERATURE)
#define __temperature_h

#include "definitions.h"
#include "atoll_preferences.h"
#include "atoll_temperature_sensor.h"
#include "temperature_compensation.h"

class Temperature : public Atoll::Preferences {
   public:
    typedef Atoll::TemperatureSensor Sensor;

    Sensor *crankSensor = nullptr;
    TemperatureCompensation *tc = nullptr;

    Temperature();
    ~Temperature();

    void setup();
    void begin();

    void onSensorValueChange(Sensor *sensor);

    bool setCompensationOffset();
    float getCompensation();

   protected:
    void setCompensation(float temperature);

    float compensationOffset = 0.0f;  // kg
    float compensation = 0.0f;        // kg
};

#endif