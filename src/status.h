#ifndef STATUS_H
#define STATUS_H

#include <Arduino.h>
#include "task.h"

class Status : public Task {
   public:
    bool statusEnabled = false;
    uint32_t statusDelay = 1000;  // ms

    void setup();
    void loop();
    char getChar();
    int getStr(char *str, int maxLength);
    int getStr(char *str, int maxLength, bool echo);
    void setFreq(float freq);
    void printHeader();
    void print();
    void handleInput(const char input);
};

#endif