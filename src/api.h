#ifndef API_H
#define API_H

#include "definitions.h"
#include "atoll_api.h"
#include "ble_server.h"

class Api : public Atoll::Api {
   public:
    static void setup(Api *instance,
                      ::Preferences *p,
                      const char *preferencesNS,
                      const char *serviceUuid = nullptr);
    virtual void beforeBleServiceStart(BLEService *service) override;

   protected:
    static Result *systemProcessor(Message *);
    static Result *weightServiceProcessor(Message *);
    static Result *calibrateStrainProcessor(Message *);
    static Result *tareProcessor(Message *);
    static Result *crankLengthProcessor(Message *);
    static Result *reverseStrainProcessor(Message *);
    static Result *doublePowerProcessor(Message *);
    static Result *sleepDelayProcessor(Message *);
    static Result *hallCharProcessor(Message *);
    static Result *hallOffsetProcessor(Message *);
    static Result *hallThresholdProcessor(Message *);
    static Result *hallThresLowProcessor(Message *);
    static Result *strainThresholdProcessor(Message *);
    static Result *strainThresLowProcessor(Message *);
    static Result *motionDetectionMethodProcessor(Message *);
    static Result *sleepProcessor(Message *);
    static Result *negativeTorqueMethodProcessor(Message *);
    static Result *autoTareProcessor(Message *);
    static Result *autoTareDelayMsProcessor(Message *);
    static Result *autoTareRangeGProcessor(Message *);
};

#endif
