#ifndef IDLE_H
#define IDLE_H

class Idle {
    public:
    int lastIdleCycles = 0;
    int idleCycles = 0;

    void increaseIdleCycles() {
       idleCycles++;
    }

    void resetIdleCycles() {
        lastIdleCycles = idleCycles;
        idleCycles = 0;
    }
};

#endif