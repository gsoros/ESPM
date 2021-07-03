#include "serialsplitter.h"

SerialSplitter Serial;

void SerialSplitter::setup(
    Stream *stream0,
    Stream *stream1,
    bool stream0_enabled,
    bool stream1_enabled) {
    s0 = stream0;
    s0_enabled = stream0_enabled;
    s1 = stream1;
    s1_enabled = stream1_enabled;
}

int SerialSplitter::available() {
    return (s0_enabled ? 0 < s0->available() : 0) + (s1_enabled ? 0 < s1->available() : 0);
}

// Note stream1 isn't read while stream0 has data
int SerialSplitter::read() {
    return s0_enabled && s0->available()   ? s0->read()
           : s1_enabled && s1->available() ? s1->read()
                                           : -1;
}

size_t SerialSplitter::write(uint8_t c) {
    return write(&c, 1);
}

size_t SerialSplitter::write(const uint8_t *buffer, size_t size) {
    int len0 = 0, len1 = 0;
    if (s0_enabled) len0 = s0->write(buffer, size);
    if (s1_enabled) len1 = s1->write(buffer, size);
    return max(len0, len1);
}

// Note preference for stream0
int SerialSplitter::peek(void) {
    if (s0_enabled && s0->available()) {
        return s0->peek();
    }
    if (s1_enabled && s1->available()) {
        return s1->peek();
    }
    return -1;
}

void SerialSplitter::flush(void) {
    if (s0_enabled) {
        s0->flush();
    }
    if (s1_enabled) {
        s1->flush();
    }
}
