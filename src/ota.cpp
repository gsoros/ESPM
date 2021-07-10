#include "ota.h"
#include "board.h"

void OTA::setup() { setup("ESPM"); }
void OTA::setup(const char *hostName) { setup(hostName, 3232); }
void OTA::setup(const char *hostName, uint16_t port) {
    // Port defaults to 3232
    ArduinoOTA.setPort(port);

    // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setHostname(hostName);

    ArduinoOTA.setTimeout(5000);  // for choppy WiFi

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA
        .onStart([]() {
            Serial.println("[OTA] Update start");
#ifdef FEATURE_WEBSERVER
            Serial.println("[OTA] Stopping webserver task");
            board.webserver.taskStop();
#endif
            if (ArduinoOTA.getCommand() == U_FLASH)
                Serial.println("[OTA] Flash");
            else {  // U_SPIFFS
                Serial.println("[OTA] FS");
#ifdef FEATURE_WEBSERVER
                Serial.println("[OTA] Unmounting littlefs");
                LITTLEFS.end();
#endif
            }
        })
        .onEnd([]() {
            Serial.println("[OTA] End");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[OTA] Progress: %d/%d\n", progress, total);
        })
        .onError([](ota_error_t error) {
            Serial.printf("[OTA] Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
                Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR)
                Serial.println("End Failed");
        });

    ArduinoOTA.begin();
}

void OTA::loop(const ulong t) {
    ArduinoOTA.handle();
}
