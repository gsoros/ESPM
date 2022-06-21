#ifndef WIFISERIAL_H
#define WIFISERIAL_H
#error removed
#include <Arduino.h>
#include <Stream.h>
#include <WiFi.h>

#include "CircularBuffer.h"
#include "task.h"
#include "definitions.h"

#ifndef WIFISERIAL_RINGBUF_RX_SIZE
#define WIFISERIAL_RINGBUF_RX_SIZE 64
#endif
#ifndef WIFISERIAL_RINGBUF_TX_SIZE
#define WIFISERIAL_RINGBUF_TX_SIZE 128
#endif
#ifndef WIFISERIAL_PORT
#define WIFISERIAL_PORT 23
#endif
#ifndef WIFISERIAL_MAXCLIENTS
#define WIFISERIAL_MAXCLIENTS 1  // max 1 client as not async
#endif

class WifiSerial : public Task, public Stream {
   public:
    void setup();
    void setup(uint16_t port, uint8_t maxClients);
    void loop();
    void off();
    void disconnect();
    size_t write(uint8_t c);
    size_t write(const uint8_t *buf, size_t size);

    int available();
    int read();
    int peek();
    void flush();

   private:
    WiFiServer _server;
    WiFiClient _client;
    CircularBuffer<char, WIFISERIAL_RINGBUF_RX_SIZE> _rx_buf;
    CircularBuffer<char, WIFISERIAL_RINGBUF_TX_SIZE> _tx_buf;
    bool _connected = false;
    bool _disconnect = false;
};
#endif