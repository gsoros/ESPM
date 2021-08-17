#ifndef WIFICONNECTION_H
#define WIFICONNECTION_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

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

    void setup(Preferences *p, const char *preferencesNS = "WiFi") {
        preferencesSetup(p, preferencesNS);
        loadDefaultSettings();
        loadSettings();
        applySettings();
    }

    void loop() {}

    void off() {
        Serial.println("[Wifi] Shutting down");
        settings.enabled = false;
        applySettings();
        taskStop();
    }

    void loadSettings() {
        if (!preferencesStartLoad()) return;
        settings.enabled = preferences->getBool("enabled", false);
        settings.apEnabled = preferences->getBool("apEnabled", false);
        //preferences->getBytes("apSSID", settings.apSSID, 32);
        //preferences->getBytes("apPassword", settings.apPassword, 32);
        strncpy(settings.apSSID, preferences->getString("apSSID").c_str(), 32);
        strncpy(settings.apPassword, preferences->getString("apPassword").c_str(), 32);
        settings.staEnabled = preferences->getBool("staEnabled", false);
        //preferences->getBytes("staSSID", settings.staSSID, 32);
        //preferences->getBytes("staPassword", settings.staPassword, 32);
        strncpy(settings.staSSID, preferences->getString("staSSID").c_str(), 32);
        strncpy(settings.staPassword, preferences->getString("staPassword").c_str(), 32);
        preferencesEnd();
    }

    void loadDefaultSettings() {
        settings.enabled = true;
        settings.apEnabled = true;
        strncpy(settings.apSSID, HOSTNAME, 32);
        strncpy(settings.apPassword, "", 32);
        settings.staEnabled = false;
        strncpy(settings.staSSID, "", 32);
        strncpy(settings.staPassword, "", 32);
    }

    void saveSettings() {
        if (!preferencesStartSave()) return;
        preferences->putBool("enabled", settings.enabled);
        preferences->putBool("apEnabled", settings.apEnabled);
        preferences->putString("apSSID", settings.apSSID);
        preferences->putString("apPassword", settings.apPassword);
        preferences->putBool("staEnabled", settings.staEnabled);
        preferences->putString("staSSID", settings.staSSID);
        preferences->putString("staPassword", settings.staPassword);
        preferencesEnd();
    }

    void printSettings() {
        Serial.printf("[Wifi] Wifi %sabled\n",
                      settings.enabled ? "En" : "Dis");
        printAPSettings();
        printSTASettings();
    }

    void printAPSettings() {
        Serial.printf("[Wifi] AP %s '%s' '%s'\n",
                      settings.apEnabled ? "Enabled" : "Disabled",
                      settings.apSSID,
                      "***"  //settings.apPassword
        );
        if (settings.apEnabled)
            Serial.printf("[Wifi] AP online, IP: %s\n", WiFi.softAPIP().toString().c_str());
    }

    void printSTASettings() {
        Serial.printf("[Wifi] STA %s '%s' '%s'\n",
                      settings.staEnabled ? "Enabled" : "Disabled",
                      settings.staSSID,
                      "***"  //settings.staPassword
        );
        if (WiFi.isConnected())
            Serial.printf("[Wifi] STA connected, local IP: %s\n", WiFi.localIP().toString().c_str());
    }

    void applySettings() {
        Serial.println("[Wifi] Applying settings, connection might need to be reset");
        Serial.flush();
        delay(1000);
        if (settings.enabled && (settings.apEnabled || settings.staEnabled)) {
            if (settings.apEnabled && settings.staEnabled)
                WiFi.mode(WIFI_MODE_APSTA);
            else if (settings.apEnabled)
                WiFi.mode(WIFI_MODE_AP);
            else
                WiFi.mode(WIFI_MODE_STA);
        } else
            WiFi.mode(WIFI_MODE_NULL);
        if (settings.enabled && settings.apEnabled) {
            if (0 == strcmp("", const_cast<char *>(settings.apSSID))) {
                log_e("Warning: cannot enable AP with empty SSID\n");
                settings.apEnabled = false;
            } else {
                log_i("Setting up WiFi AP '%s'\n", settings.apSSID);
                WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
                    log_i("WiFi AP new connection, now active: %d\n", WiFi.softAPgetStationNum());
                },
                             SYSTEM_EVENT_AP_STACONNECTED);
                WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
                    log_i("WiFi AP station disconnected, now active: %d\n", WiFi.softAPgetStationNum());
                },
                             SYSTEM_EVENT_AP_STADISCONNECTED);
                WiFi.softAP(settings.apSSID, settings.apPassword);
            }
        }
        if (settings.enabled && settings.staEnabled) {
            if (0 == strcmp("", const_cast<char *>(settings.staSSID))) {
                log_e("Warning: cannot enable STA with empty SSID\n");
                settings.staEnabled = false;
            } else {
                log_i("Connecting WiFi STA to AP '%s'\n", settings.staSSID);
                WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
                    log_i("WiFi STA connected, IP: %s\n", WiFi.localIP().toString().c_str());
                },
                             SYSTEM_EVENT_STA_GOT_IP);
                WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
                    log_i("WiFi STA disconnected\n");
                },
                             SYSTEM_EVENT_STA_LOST_IP);
                WiFi.begin(settings.staSSID, settings.staPassword);
            }
        }
    }

    void setEnabled(bool state) {
        settings.enabled = state;
        applySettings();
        saveSettings();
    }

    bool isEnabled() {
        return settings.enabled;
    }

    bool connected() {
        return WiFi.isConnected() || WiFi.softAPgetStationNum() > 0;
    }
};

#endif