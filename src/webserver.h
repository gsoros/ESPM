#ifndef WEBSERVER_H____
#define WEBSERVER_H____

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LITTLEFS.h>
#include <WiFi.h>

#include "mpu.h"
#include "task.h"

class WebServer : public Task {
   public:
    AsyncWebServer *ws;
    MPU *mpu;

    AsyncWebSocket *wss;
    StaticJsonDocument<96> json;
    int wssConnectedCount = 0;
    bool wssBroadcastEnabled = false;
    ulong wssLastBroadcast = 0;
    ulong wssLastCleanup = 0;
    float strain = 0.0;
    float qX = 0.0;
    float qY = 0.0;
    float qZ = 0.0;
    float qW = 0.0;

    void setup(MPU *m) {
        mpu = m;
        if (!LITTLEFS.begin())
            log_e("Could not mount LITTLEFS");
        wss = new AsyncWebSocket("/ws");
        wss->onEvent([this](AsyncWebSocket *server,
                            AsyncWebSocketClient *client,
                            AwsEventType type,
                            void *arg,
                            uint8_t *data,
                            size_t len) { handleWebSocketEvent(server,
                                                               client,
                                                               type,
                                                               arg,
                                                               data,
                                                               len); });
        ws = new AsyncWebServer(80);
        ws->addHandler(wss);
        ws->on("/calibrateAccelGyro", [this](AsyncWebServerRequest *request) { handleCalibrateAccelGyro(request); });
        ws->on("/calibrateMag", [this](AsyncWebServerRequest *request) { handleCalibrateMag(request); });
        ws->on("/reboot", [this](AsyncWebServerRequest *request) { handleReboot(request); });
        ws->on("/favicon.ico", [](AsyncWebServerRequest *request) { request->send(204); });
        AsyncStaticWebHandler *aswh = &ws->serveStatic("/", LITTLEFS, "/").setDefaultFile("index.html");
        time_t lastModified = (time_t)0;
        if (!(lastModified = getLastModified())) {
#ifdef COMPILE_TIMESTAMP
            log_i("Setting last modified to %d\n", COMPILE_TIMESTAMP);
            lastModified = COMPILE_TIMESTAMP;
#endif
        }
        if (lastModified)
            aswh->setLastModified((struct tm *)gmtime(&lastModified));
        ws->begin();
        wssBroadcastEnabled = true;
    }

    void loop(const ulong t) {
        if (wssLastCleanup < t - 1000) {
            wss->cleanupClients();
            wssLastCleanup = t;
        }
        if (wssBroadcastEnabled && webSocketConnected() && (wssLastBroadcast < t - 100)) {
            webSocketBroadcast();
            wssLastBroadcast = t;
        }
    }

    void handleCalibrateAccelGyro(AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Calibrating accel/gyro, device should be motionless.");
        //wssBroadcastEnabled = false;
        mpu->accelGyroNeedsCalibration = true;
        //wssBroadcastEnabled = true;
    }

    void handleCalibrateMag(AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Calibrating magnetometer, wave device around gently for 15 seconds.");
        //wssBroadcastEnabled = false;
        mpu->magNeedsCalibration = true;
        //wssBroadcastEnabled = true;
    }

    void handleReboot(AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Rebooting...");
        log_i("Rebooting...");
        ESP.restart();
    }

    void webSocketBroadcast() {
        char output[100];
        json.clear();
        json["strain"] = strain;
        json["qX"] = qX;
        json["qY"] = qY;
        json["qZ"] = qZ;
        json["qW"] = qW;
        serializeJson(json, output);
        wss->textAll(output);
    }

    bool webSocketConnected() {
        return wssConnectedCount > 0;
    }

    void handleWebSocketEvent(
        AsyncWebSocket *server,
        AsyncWebSocketClient *client,
        AwsEventType type,
        void *arg,
        uint8_t *data,
        size_t len) {
        if (type == WS_EVT_CONNECT) {
            wssConnectedCount++;
        } else if (type == WS_EVT_DISCONNECT) {
            wssConnectedCount--;
        }
    }

    time_t getLastModified() {
        time_t t = (time_t)0;
        File file = LITTLEFS.open("/lastmodified");
        if (!file || file.isDirectory()) {
            file.close();
            return t;
        }
        char buf[11];
        file.readBytes(buf, 10);
        file.close();
        buf[10] = '\0';
        t = (time_t)atoi(buf);
        log_i("From FS: %d\n", t);
        return t;
    }
};

#endif