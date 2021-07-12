#if !defined(SPLITSTREAM_H) && defined(FEATURE_SERIAL)
#define SPLITSTREAM_H

#include <Arduino.h>
#include <Stream.h>

// Splits one stream into two
class SplitStream : public Stream {
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
    operator bool() const;
};

#if !defined(NO_GLOBAL_SPLITSTREAM) && !defined(NO_GLOBAL_INSTANCES)
extern SplitStream Serial;
#endif

#endif