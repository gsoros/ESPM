#ifndef HASPREFERENCES_H
#define HASPREFERENCES_H

#include <Arduino.h>
#include <Preferences.h>

class HasPreferences {
   public:
    Preferences *preferences;
    const char *preferencesNS;

    void preferencesSetup(Preferences *p, const char *ns) {
        preferences = p;
        preferencesNS = ns;
    }

    bool preferencesStartLoad() {
        if (!preferences->begin(preferencesNS, true))  // try ro mode
        {
            if (!preferences->begin(preferencesNS, false))  // open in rw mode to create ns
            {
                log_e("Preferences begin failed for '%s'\n", preferencesNS);
                return false;
            }
        }
        return true;
    };

    bool preferencesStartSave() {
        if (!preferences->begin(preferencesNS, false)) {
            log_e("Preferences begin failed for '%s'.", preferencesNS);
            return false;
        }
        return true;
    };

    void preferencesEnd() {
        preferences->end();
    }
};

#endif