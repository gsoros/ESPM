#ifndef API_H
#define API_H

#include "definitions.h"
#include "atoll_api.h"
#include "ble_server.h"

typedef Atoll::ApiResult ApiResult;
typedef Atoll::ApiMessage ApiMessage;

typedef ApiResult *(*ApiProcessor)(ApiMessage *reply);

class ApiCommand : public Atoll::ApiCommand {
   public:
    ApiCommand(
        const char *name = "",
        ApiProcessor processor = nullptr,
        uint8_t code = 0)
        : Atoll::ApiCommand(name, processor, code) {}
};

class Api : public Atoll::Api {
   public:
    static void setup(Api *instance,
                      ::Preferences *p,
                      const char *preferencesNS,
                      BleServer *bleServer = nullptr,
                      const char *serviceUuid = nullptr);
    virtual void beforeBleServiceStart(BLEService *service) override;
    void notifyTxChar(const char *str);

   protected:
    static ApiResult *systemProcessor(ApiMessage *);
    static ApiResult *weightServiceProcessor(ApiMessage *);
    static ApiResult *calibrateStrainProcessor(ApiMessage *);
    static ApiResult *tareProcessor(ApiMessage *);
    static ApiResult *crankLengthProcessor(ApiMessage *);
    static ApiResult *reverseStrainProcessor(ApiMessage *);
    static ApiResult *doublePowerProcessor(ApiMessage *);
    static ApiResult *sleepDelayProcessor(ApiMessage *);
    static ApiResult *hallCharProcessor(ApiMessage *);
    static ApiResult *hallOffsetProcessor(ApiMessage *);
    static ApiResult *hallThresholdProcessor(ApiMessage *);
    static ApiResult *hallThresLowProcessor(ApiMessage *);
    static ApiResult *strainThresholdProcessor(ApiMessage *);
    static ApiResult *strainThresLowProcessor(ApiMessage *);
    static ApiResult *motionDetectionMethodProcessor(ApiMessage *);
    static ApiResult *sleepProcessor(ApiMessage *);
    static ApiResult *negativeTorqueMethodProcessor(ApiMessage *);
    static ApiResult *autoTareProcessor(ApiMessage *);
    static ApiResult *autoTareDelayMsProcessor(ApiMessage *);
    static ApiResult *autoTareRangeGProcessor(ApiMessage *);
};

#endif
