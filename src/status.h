#ifndef STATUS_H
#define STATUS_H

#include <Arduino.h>
#include "task.h"

class Status : public Task {
   public:
    bool statusEnabled = false;
    uint32_t statusDelay = 1000;  // ms

    void setup();
    void loop(const ulong t);
    char getChar();
    int getStr(char *str, int maxLength);
    int getStr(char *str, int maxLength, bool echo);
    void setStatusFreq(float freq);
    void printStatus(const ulong t);
    void handleInput(const char input);
};

#endif