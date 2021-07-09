#include "wifiserial.h"
#include "board.h"

void WifiSerial::setup() { setup(23); }
void WifiSerial::setup(uint16_t port) {
    _server = WiFiServer(port, 1);  // max 1 client as not async
    _server.begin();
}

void WifiSerial::loop(const ulong t) {
    if (!WiFi.isConnected() && WiFi.softAPgetStationNum() < 1) return;
    if (!_connected) {
        if (!_server.hasClient()) return;
        _client = _server.available();
        if (!_client) return;
        _connected = true;
        Serial.print("WifiSerial client connected\n");
        board.led.blink(5);
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

void WifiSerial::disconnect() {
    if (_client.connected()) {
        _client.write(7);  // BEL
        _client.write(4);  // EOT
        _client.write('\n');
    }
    _client.stop();
    _connected = false;
    Serial.print("WifiSerial client disconnected\n");
    board.led.blink(5);
}

size_t WifiSerial::write(uint8_t c) {
    return write(&c, 1);
}

size_t WifiSerial::write(const uint8_t *buf, size_t size) {
    // TODO mutex
    size_t written = size;
    while (size) {
        _tx_buf.push(*buf++);
        size--;
    }
    return written;
}

int WifiSerial::available() {
    return _rx_buf.size();
}

int WifiSerial::read() {
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

int WifiSerial::peek() {
    return _rx_buf.first();
}

void WifiSerial::flush() {
    _client.flush();
}
