#ifndef WEBSERVER_H____
#define WEBSERVER_H____

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LITTLEFS.h>
#include <ArduinoJson.h>
#include "mpu.h"

class WebServer
{
public:
    AsyncWebServer *ws;
    MPU *mpu;

    AsyncWebSocket *wss;
    StaticJsonDocument<200> json;
    int wssConnectedCount = 0;
    bool wssBroadcastEnabled = false;
    ulong wssLastBroadcast = 0;
    ulong wssLastCleanup = 0;
    float strain = 0.0;
    float qX = 0.0;
    float qY = 0.0;
    float qZ = 0.0;
    float qW = 0.0;

    void setup(MPU *m)
    {
        mpu = m;
        if (!LITTLEFS.begin())
            log_e("Could not mount LITTLEFS");
        wss = new AsyncWebSocket("/ws");
        wss->onEvent([this](AsyncWebSocket *server,
                            AsyncWebSocketClient *client,
                            AwsEventType type,
                            void *arg,
                            uint8_t *data,
                            size_t len)
                     { handleWebSocketEvent(server,
                                            client,
                                            type,
                                            arg,
                                            data,
                                            len); });
        ws = new AsyncWebServer(80);
        ws->addHandler(wss);
        ws->on("/calibrateAccelGyro", [this](AsyncWebServerRequest *request)
               { handleCalibrateAccelGyro(request); });
        ws->on("/calibrateMag", [this](AsyncWebServerRequest *request)
               { handleCalibrateMag(request); });
        ws->on("/reboot", [this](AsyncWebServerRequest *request)
               { handleReboot(request); });
        ws->serveStatic("/", LITTLEFS, "/").setDefaultFile("index.html");
        ws->begin();
        wssBroadcastEnabled = true;
    }

    void loop(const ulong t)
    {
        if (wssLastCleanup < t - 1000)
        {
            wss->cleanupClients();
            wssLastCleanup = t;
        }
        if (wssBroadcastEnabled && webSocketConnected() && (wssLastBroadcast < t - 100))
        {
            webSocketBroadcast();
            wssLastBroadcast = t;
        }
    }

    void handleCalibrateAccelGyro(AsyncWebServerRequest *request)
    {
        request->send(200, "text/plain", "Calibrating accel/gyro, device should be motionless.");
        wssBroadcastEnabled = false;
        mpu->accelGyroNeedsCalibration = true;
        wssBroadcastEnabled = true;
    }

    void handleCalibrateMag(AsyncWebServerRequest *request)
    {
        request->send(200, "text/plain", "Calibrating magnetometer, wave device around gently for 15 seconds.");
        wssBroadcastEnabled = false;
        mpu->magNeedsCalibration = true;
        wssBroadcastEnabled = true;
    }

    void handleReboot(AsyncWebServerRequest *request)
    {
        Serial.println("Rebooting...");
        ESP.restart();
    }

    void webSocketBroadcast()
    {
        char output[200];
        json.clear();
        json["strain"] = strain;
        json["qX"] = qX;
        json["qY"] = qY;
        json["qZ"] = qZ;
        json["qW"] = qW;
        serializeJson(json, output);
        wss->textAll(output);
    }

    bool webSocketConnected()
    {
        return wssConnectedCount > 0;
    }

    void handleWebSocketEvent(
        AsyncWebSocket *server,
        AsyncWebSocketClient *client,
        AwsEventType type,
        void *arg,
        uint8_t *data,
        size_t len)
    {
        if (type == WS_EVT_CONNECT)
        {
            wssConnectedCount++;
            log_i("ws[%s][%u] connect\n", server->url(), client->id());
            client->printf("Hello Client %u :)", client->id());
            client->ping();
        }
        else if (type == WS_EVT_DISCONNECT)
        {
            wssConnectedCount--;
            log_i("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
        }
        else if (type == WS_EVT_ERROR)
        {
            //error was received from the other end
            log_i("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
        }
        else if (type == WS_EVT_PONG)
        {
            //pong message was received (in response to a ping request maybe)
            log_i("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
        }
        else if (type == WS_EVT_DATA)
        {
            //data packet
            AwsFrameInfo *info = (AwsFrameInfo *)arg;
            if (info->final && info->index == 0 && info->len == len)
            {
                //the whole message is in a single frame and we got all of it's data
                log_i("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);
                if (info->opcode == WS_TEXT)
                {
                    data[len] = 0;
                    log_i("%s\n", (char *)data);
                }
                else
                {
                    for (size_t i = 0; i < info->len; i++)
                    {
                        log_i("%02x ", data[i]);
                    }
                    log_i("\n");
                }
                if (info->opcode == WS_TEXT)
                    client->text("I got your text message");
                else
                    client->binary("I got your binary message");
            }
            else
            {
                //message is comprised of multiple frames or the frame is split into multiple packets
                if (info->index == 0)
                {
                    if (info->num == 0)
                        log_i("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                    log_i("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
                }

                log_i("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);
                if (info->message_opcode == WS_TEXT)
                {
                    data[len] = 0;
                    log_i("%s\n", (char *)data);
                }
                else
                {
                    for (size_t i = 0; i < len; i++)
                    {
                        log_i("%02x ", data[i]);
                    }
                    log_i("\n");
                }

                if ((info->index + len) == info->len)
                {
                    log_i("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
                    if (info->final)
                    {
                        log_i("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                        if (info->message_opcode == WS_TEXT)
                            client->text("I got your text message");
                        else
                            client->binary("I got your binary message");
                    }
                }
            }
        }
    }
};

#endif