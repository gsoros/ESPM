#include "nullserial.h"

#ifndef FEATURE_SERIAL

#if !defined(NO_GLOBAL_NULLSERIAL) && !defined(NO_GLOBAL_INSTANCES)
NullSerial Serial;
#endif

void NullSerial::setup(void) {}
int NullSerial::available(void) { return 0; }
int NullSerial::read(void) { return -1; }
size_t NullSerial::write(uint8_t c) { return 1; }
size_t NullSerial::write(const uint8_t *buffer, size_t size) { return size; }
int NullSerial::peek(void) { return -1; }
void NullSerial::flush(void) {}
NullSerial::operator bool() const { return true; }

#endif