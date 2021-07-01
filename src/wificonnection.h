#ifndef WIFICONNECTION_H
#define WIFICONNECTION_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

#include "haspreferences.h"

typedef struct
{
    bool apEnable;
    char apSSID[32];
    char apPassword[32];
    bool staEnable;
    char staSSID[32];
    char staPassword[32];
} WifiConnectionSettings;

class WifiConnection : public HasPreferences {
   public:
    WifiConnectionSettings settings;

    void setup(Preferences *p, const char *preferencesNS = "WiFi") {
        preferencesSetup(p, preferencesNS);
        if (!loadSettings())
            loadDefaultSettings();
        applySettings();
    }

    void loop(const ulong t) {
    }

    bool loadSettings() {
        if (!preferencesStartLoad()) return false;
        settings.apEnable = preferences->getBool("apEnable", false);
        //preferences->getBytes("apSSID", settings.apSSID, 32);
        //preferences->getBytes("apPassword", settings.apPassword, 32);
        strncpy(settings.apSSID, preferences->getString("apSSID").c_str(), 32);
        strncpy(settings.apPassword, preferences->getString("apPassword").c_str(), 32);
        settings.staEnable = preferences->getBool("staEnable", false);
        //preferences->getBytes("staSSID", settings.staSSID, 32);
        //preferences->getBytes("staPassword", settings.staPassword, 32);
        strncpy(settings.staSSID, preferences->getString("staSSID").c_str(), 32);
        strncpy(settings.staPassword, preferences->getString("staPassword").c_str(), 32);
        preferencesEnd();
        if (!settings.staEnable && !settings.apEnable)
            return false;
        return true;
    }

    void loadDefaultSettings() {
        settings.apEnable = true;
        strncpy(settings.apSSID, "ESPM", 32);
        strncpy(settings.apPassword, "", 32);
        settings.staEnable = false;
        strncpy(settings.staSSID, "", 32);
        strncpy(settings.staPassword, "", 32);
    }

    void saveSettings() {
        if (!preferencesStartSave()) return;
        preferences->putBool("apEnable", settings.apEnable);
        preferences->putString("apSSID", settings.apSSID);
        preferences->putString("apPassword", settings.apPassword);
        preferences->putBool("staEnable", settings.staEnable);
        preferences->putString("staSSID", settings.staSSID);
        preferences->putString("staPassword", settings.staPassword);
        preferencesEnd();
    }

    void printSettings() {
        printAPSettings();
        printSTASettings();
    }

    void printAPSettings() {
        Serial.printf("WiFi AP %s '%s' '%s'\n",
                      settings.apEnable ? "Enabled" : "Disabled",
                      settings.apSSID,
                      "***"  //settings.apPassword
        );
        if (settings.apEnable)
            Serial.printf("AP online, IP: %s\n", WiFi.softAPIP().toString().c_str());
    }

    void printSTASettings() {
        Serial.printf("WiFi STA %s '%s' '%s'\n",
                      settings.staEnable ? "Enabled" : "Disabled",
                      settings.staSSID,
                      "***"  //settings.staPassword
        );
        if (WiFi.isConnected())
            Serial.printf("STA connected, local IP: %s\n", WiFi.localIP().toString().c_str());
    }

    void applySettings() {
        if (settings.apEnable || settings.staEnable) {
            if (settings.apEnable && settings.staEnable)
                WiFi.mode(WIFI_MODE_APSTA);
            else if (settings.apEnable)
                WiFi.mode(WIFI_MODE_AP);
            else
                WiFi.mode(WIFI_MODE_STA);
        } else
            WiFi.mode(WIFI_MODE_NULL);
        if (settings.apEnable) {
            if (0 == strcmp("", const_cast<char *>(settings.apSSID))) {
                log_e("Warning: cannot enable AP with empty SSID\n");
                settings.apEnable = false;
            } else {
                log_i("Setting up WiFi AP '%s'\n", settings.apSSID);
                WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) { log_i("WiFi AP new connection, now active: %d\n",
                                                                                 WiFi.softAPgetStationNum()); },
                             SYSTEM_EVENT_AP_STACONNECTED);
                WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) { log_i("WiFi AP station disconnected, now active: %d\n",
                                                                                 WiFi.softAPgetStationNum()); },
                             SYSTEM_EVENT_AP_STADISCONNECTED);
                WiFi.softAP(settings.apSSID, settings.apPassword);
            }
        }
        if (settings.staEnable) {
            if (0 == strcmp("", const_cast<char *>(settings.staSSID))) {
                log_e("Warning: cannot enable STA with empty SSID\n");
                settings.staEnable = false;
            } else {
                log_i("Connecting WiFi STA to AP '%s'\n", settings.staSSID);
                WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) { log_i("WiFi STA connected, IP: %s\n",
                                                                                 WiFi.localIP().toString().c_str()); },
                             SYSTEM_EVENT_STA_GOT_IP);
                WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) { log_i("WiFi STA disconnected\n"); },
                             SYSTEM_EVENT_STA_LOST_IP);
                WiFi.begin(settings.staSSID, settings.staPassword);
            }
        }
    }

    bool connected() {
        //return WiFi.softAPgetStationNum() > 0;
        return WiFi.isConnected();
    }
};

#endif