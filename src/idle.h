#ifndef IDLE_H
#define IDLE_H

class Idle {
   public:
    unsigned int lastIdleCycles = 0;
    unsigned int idleCycles = 0;
    unsigned int idleCyclesMax = 0;

    void increaseIdleCycles() {
        idleCycles++;
        if (idleCycles > idleCyclesMax) {
            idleCyclesMax = idleCycles;
        }
    }

    void resetIdleCycles() {
        lastIdleCycles = idleCycles;
        idleCycles = 0;
    }
};

#endif