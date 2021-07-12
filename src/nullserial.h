#if !defined(NULLSERIAL_H) && !defined(FEATURE_SERIAL)
#define NULLSERIAL_H

#include <Arduino.h>
#include <Stream.h>

// Dummy Serial
class NullSerial : public Stream {
   public:
    void setup(void);
    int available(void);
    int read(void);
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    int peek(void);
    void flush(void);
    operator bool() const;
};

#if !defined(NO_GLOBAL_NULLSERIAL) && !defined(NO_GLOBAL_INSTANCES)
extern NullSerial Serial;
#endif

#endif