#ifndef OTA_H___
#define OTA_H___
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#error removed
#include "task.h"

class OTA : public Task {
   public:
    void setup();
    void setup(const char *hostName);
    void setup(const char *hostName, uint16_t port);
    void loop();
    void off();

   private:
    uint8_t lastProgressPercent = 0;
};
#endif