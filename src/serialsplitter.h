#ifndef SERIALSPLITTER_H
#define SERIALSPLITTER_H

#include <Arduino.h>
#include <Stream.h>

class SerialSplitter : public Stream {
   public:
    Stream *s0;
    bool s0_enabled;
    Stream *s1;
    bool s1_enabled;

    void setup(
        Stream *stream0,
        Stream *stream1,
        bool stream0_enabled,
        bool stream1_enabled);

    int available();

    int read();

    size_t write(uint8_t c);

    size_t write(const uint8_t *buffer, size_t size);

    int peek(void);

    void flush(void);
};

extern SerialSplitter Serial;

#endif