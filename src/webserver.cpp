#ifdef FEATURE_WEBSERVER

#include "webserver.h"
#include "board.h"
#include "mpu.h"
#include "power.h"
#include "strain.h"

void WebServer::setup() {
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

void WebServer::loop() {
    const ulong t = millis();
    if (wssLastCleanup < t - 300) {
        wss->cleanupClients();
        wssLastCleanup = t;
    }
    if (wssBroadcastEnabled && webSocketConnected() && (wssLastBroadcast < t - 100)) {
        webSocketBroadcast();
        wssLastBroadcast = t;
    }
}

void WebServer::handleCalibrateAccelGyro(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Calibrating accel/gyro, device should be motionless.");
    //wssBroadcastEnabled = false;
    board.mpu.accelGyroNeedsCalibration = true;
    //wssBroadcastEnabled = true;
}

void WebServer::handleCalibrateMag(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Calibrating magnetometer, wave device around gently for 15 seconds.");
    //wssBroadcastEnabled = false;
    board.mpu.magNeedsCalibration = true;
    //wssBroadcastEnabled = true;
}

void WebServer::handleReboot(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Rebooting...");
    log_i("Rebooting...");
    ESP.restart();
}

void WebServer::webSocketBroadcast() {
    char output[128];
    MPU::Quaternion q = board.mpu.quaternion();
    json.clear();
    json["qX"] = q.x;
    json["qY"] = q.y;
    json["qZ"] = q.z;
    json["qW"] = q.w;
    json["rpm"] = (int)board.getRpm();
    json["strain"] = (int)board.getStrain();
    json["power"] = (int)board.getPower();
    serializeJson(json, output);
    wss->textAll(output);
}

bool WebServer::webSocketConnected() {
    return wssConnectedCount > 0;
}

void WebServer::handleWebSocketEvent(
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

time_t WebServer::getLastModified() {
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

#endif