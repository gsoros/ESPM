#ifndef OTA_H___
#define OTA_H___
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "task.h"

class OTA : public Task {
   public:
    void setup(const char *hostName = "ESPM", uint16_t port = 3232) {
        // Port defaults to 3232
        ArduinoOTA.setPort(port);

        // Hostname defaults to esp3232-[MAC]
        ArduinoOTA.setHostname(hostName);

        // No authentication by default
        // ArduinoOTA.setPassword("admin");

        // Password can be set with it's md5 value as well
        // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
        // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

        ArduinoOTA
            .onStart([]() {
                log_i("Update start");
                if (ArduinoOTA.getCommand() == U_FLASH)
                    log_i("Flash");
                else  // U_SPIFFS
                    log_i("FS");

                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            })
            .onEnd([]() { log_i("End"); })
            .onProgress([](unsigned int progress, unsigned int total) { log_i("Progress: %d/%d", progress, total); })
            .onError([](ota_error_t error) {
                log_i("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR)
                    log_i("Auth Failed");
                else if (error == OTA_BEGIN_ERROR)
                    log_i("Begin Failed");
                else if (error == OTA_CONNECT_ERROR)
                    log_i("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR)
                    log_i("Receive Failed");
                else if (error == OTA_END_ERROR)
                    log_i("End Failed");
            });

        ArduinoOTA.begin();
    }

    void loop(const ulong t) {
        ArduinoOTA.handle();
    }
};
#endif