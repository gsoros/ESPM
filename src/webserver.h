#if !defined(WEBSERVER_H____) && defined(FEATURE_WEBSERVER)
#define WEBSERVER_H____

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LITTLEFS.h>
#include <WiFi.h>

#include "task.h"

class WebServer : public Task {
   public:
    AsyncWebServer *ws;
    AsyncWebSocket *wss;
    StaticJsonDocument<128> json;
    int wssConnectedCount = 0;
    bool wssBroadcastEnabled = false;
    ulong wssLastBroadcast = 0;
    ulong wssLastCleanup = 0;

    void setup();
    void loop();

    void handleCalibrateAccelGyro(AsyncWebServerRequest *request);
    void handleCalibrateMag(AsyncWebServerRequest *request);
    void handleReboot(AsyncWebServerRequest *request);
    void webSocketBroadcast();
    bool webSocketConnected();
    void handleWebSocketEvent(
        AsyncWebSocket *server,
        AsyncWebSocketClient *client,
        AwsEventType type,
        void *arg,
        uint8_t *data,
        size_t len);
    time_t getLastModified();
};

#endif