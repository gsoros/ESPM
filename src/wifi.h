#ifndef __wifi_h
#define __wifi_h

#include "definitions.h"
#include "atoll_wifi.h"

class Wifi : public Atoll::Wifi {
   public:
    bool autoStartOta = false;
#ifdef FEATURE_SERIAL
    bool autoStartWifiSerial = true;
#endif
    virtual void onEvent(arduino_event_id_t event, arduino_event_info_t info) override;
};

#endif