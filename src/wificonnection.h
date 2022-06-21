#ifndef WIFICONNECTION_H
#define WIFICONNECTION_H
#error removed
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

#include "definitions.h"
#include "haspreferences.h"
#include "task.h"

class WifiConnection : public HasPreferences, public Task {
   public:
    typedef struct
    {
        bool enabled;
        bool apEnabled;
        char apSSID[SETTINGS_STR_LENGTH];
        char apPassword[SETTINGS_STR_LENGTH];
        bool staEnabled;
        char staSSID[SETTINGS_STR_LENGTH];
        char staPassword[SETTINGS_STR_LENGTH];
    } ConnectionSettings;

    ConnectionSettings settings;

    void setup(Preferences *p, const char *preferencesNS = "WiFi");
    void loop();
    void off();
    void loadSettings();
    void loadDefaultSettings();
    void saveSettings();
    void printSettings();
    void printAPSettings();
    void printSTASettings();
    void applySettings();
    void setEnabled(bool state);
    bool isEnabled();
    bool connected();
    void registerCallbacks();
    void onEvent(arduino_event_id_t event, arduino_event_info_t info);
};

#endif