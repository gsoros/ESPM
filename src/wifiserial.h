#ifndef WIFISERIAL_H
#define WIFISERIAL_H

#include <Arduino.h>
#include <WiFi.h>

#include "CircularBuffer.h"
#include "Stream.h"
#include "task.h"

class WifiSerial : public Task, public Stream {
   public:
    void setup(uint16_t port = 23) {
        _server = WiFiServer(port, 1);  // max 1 client as not async
        _server.begin();
    }

    void loop(const ulong t) {
        if (!WiFi.isConnected() && WiFi.softAPgetStationNum() < 1) return;
        if (!_connected) {
            if (!_server.hasClient()) return;
            _client = _server.available();
            if (!_client) return;
            _connected = true;
            log_d("Client connected");
            _client.printf("Welcome.\n");
        } else if (!_client.connected()) {
            _client.stop();
            _connected = false;
            log_d("Client disconnected");
            return;
        }
        while (0 < _client.available()) {
            char c = _client.read();
            _rx_buf.push(c);
        }
        while (0 < _tx_buf.size()) {
            _client.write(_tx_buf.shift());
        }
    }

    void disconnect() {
        _client.stop();
        _connected = false;
        log_d("Client disconnected");
    }

    size_t write(uint8_t c) {
        return write(&c, 1);
    }

    size_t write(const uint8_t *buf, size_t size) {
        // todo mutex
        size_t written = size;
        while (size) {
            _tx_buf.push(*buf++);
            size--;
        }
        return written;
    }

    int available() {
        return _rx_buf.size();
    };

    int read() {
        if (available()) {
            return _rx_buf.shift();
        }
        return -1;
    };

    int peek() { return 0; };
    void flush(){};

   private:
    WiFiServer _server;
    WiFiClient _client;
    CircularBuffer<char, 256> _rx_buf;
    CircularBuffer<char, 256> _tx_buf;
    bool _connected = false;
};
#endif