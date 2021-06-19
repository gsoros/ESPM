#ifndef WEBSOCKETS_H
#define WEBSOCKETS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>

class Websockets
{
public:
    WebSocketsServer *wss;
    StaticJsonDocument<200> json;
    int connectedCount = 0;
    uint8_t lastMessageSent = 0;
    float strain = 0;
    float qX = 0.0;
    float qY = 0.0;
    float qZ = 0.0;
    float qW = 0.0;

    void setup()
    {
        wss = new WebSocketsServer(8001);
        wss->begin();
        wss->onEvent(
            std::bind(
                &Websockets::wsEvent,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4));
    }

    void loop(const ulong t)
    {
        wss->loop();
        if (connected() && (lastMessageSent < t - 100)) {
            send();
        }
    }

    void send()
    {
        char output[200];
        json.clear();
        json["strain"] = strain;
        json["qX"] = qX;
        json["qY"] = qY;
        json["qZ"] = qZ;
        json["qW"] = qW;
        serializeJson(json, output);
        wss->broadcastTXT(output);
    }

    bool connected()
    {
        return connectedCount > 0;
    }

    void wsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
    {
        switch (type) {
        case WStype_DISCONNECTED:
            connectedCount--;
            log_i("[%u] Disconnected\n", num);
            break;
        case WStype_CONNECTED: {
            connectedCount++;
            log_i("[%u] Connected from %s url: %s\n", num, wss->remoteIP(num).toString().c_str(), payload);

            // send message to client
            wss->sendTXT(num, "Connected");
        } break;
        case WStype_TEXT:
            log_i("[%u] get Text: %s\n", num, payload);

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            log_i("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
        case WStype_PING:
        case WStype_PONG:
            log_i("Unhandled wsEvent %i", type);
            break;
        }
    }

    void hexdump(const void *mem, uint32_t len, uint8_t cols = 16)
    {
        const uint8_t *src = (const uint8_t *)mem;
        Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
        for (uint32_t i = 0; i < len; i++) {
            if (i % cols == 0) {
                Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
            }
            Serial.printf("%02X ", *src);
            src++;
        }
        Serial.printf("\n");
    }
};

#endif