#include "splitstream.h"

#ifdef FEATURE_SERIAL

#if !defined(NO_GLOBAL_SPLITSTREAM) && !defined(NO_GLOBAL_INSTANCES)
SplitStream Serial;
#endif

void SplitStream::setup(
    Stream *stream0,
    Stream *stream1,
    bool stream0_enabled,
    bool stream1_enabled) {
    s0 = stream0;
    s0_enabled = stream0_enabled;
    s1 = stream1;
    s1_enabled = stream1_enabled;
}

int SplitStream::available() {
    int available = 0;
    if (s0_enabled) available = s0->available();
    if (s1_enabled) available += s1->available();
    return available;
}

// Note stream1 isn't read while stream0 has data
int SplitStream::read() {
    if (s0_enabled && s0->available()) return s0->read();
    if (s1_enabled && s1->available()) return s1->read();
    return -1;
}

size_t SplitStream::write(uint8_t c) {
    return write(&c, 1);
}

size_t SplitStream::write(const uint8_t *buffer, size_t size) {
    size_t len0 = 0, len1 = 0;
    if (s0_enabled) len0 = s0->write(buffer, size);
    if (s1_enabled) len1 = s1->write(buffer, size);
    return max(len0, len1);
}

// Note preference for stream0
int SplitStream::peek(void) {
    if (s0_enabled && s0->available()) return s0->peek();
    if (s1_enabled && s1->available()) return s1->peek();
    return -1;
}

void SplitStream::flush(void) {
    if (s0_enabled) s0->flush();
    if (s1_enabled) s1->flush();
}

SplitStream::operator bool() const {
    bool r0 = false, r1 = false;
    if (s0_enabled) r0 = s0 ? true : false;
    if (s1_enabled) r1 = s1 ? true : false;
    return r0 || r1;
}

#endif