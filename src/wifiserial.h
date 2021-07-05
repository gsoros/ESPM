#ifndef WIFISERIAL_H
#define WIFISERIAL_H

#include <Arduino.h>
#include <Stream.h>
#include <WiFi.h>

#include "CircularBuffer.h"
#include "task.h"

#ifndef WIFISERIAL_RINGBUF_RX_SIZE
#define WIFISERIAL_RINGBUF_RX_SIZE 64
#endif
#ifndef WIFISERIAL_RINGBUF_TX_SIZE
#define WIFISERIAL_RINGBUF_TX_SIZE 128
#endif

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
            Serial.print("WifiSerial client connected\n");
            _client.print("Welcome.\n");
        } else if (!_client.connected()) {
            disconnect();
            return;
        }
        while (0 < _client.available()) {
            _rx_buf.push(_client.read());
        }
        while (0 < _tx_buf.size()) {
            _client.write(_tx_buf.shift());
        }
        if (_disconnect) {
            flush();
            _disconnect = false;
            disconnect();
        }
    }

    void disconnect() {
        _client.stop();
        _connected = false;
        Serial.print("WifiSerial client disconnected\n");
    }

    size_t write(uint8_t c) {
        return write(&c, 1);
    }

    size_t write(const uint8_t *buf, size_t size) {
        // TODO mutex
        size_t written = size;
        while (size) {
            _tx_buf.push(*buf++);
            size--;
        }
        return written;
    }

    int available() {
        return _rx_buf.size();
    }

    int read() {
        if (available()) {
            char c = _rx_buf.shift();
            switch (c) {
                case 4:
                    print("Control-D received\nBye.\n");
                    _disconnect = true;
                    //return -1;
            }
            return c;
        }
        return -1;
    }

    int peek() {
        return _rx_buf.first();
    }

    void flush() {
        _client.flush();
    }

   private:
    WiFiServer _server;
    WiFiClient _client;
    CircularBuffer<char, WIFISERIAL_RINGBUF_RX_SIZE> _rx_buf;
    CircularBuffer<char, WIFISERIAL_RINGBUF_TX_SIZE> _tx_buf;
    bool _connected = false;
    bool _disconnect = false;
};
#endif