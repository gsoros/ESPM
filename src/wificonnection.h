#ifndef WIFICONNECTION_H
#define WIFICONNECTION_H

#include "mpu.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

typedef struct {
    bool apEnable;
    char apSSID[32];
    char apPassword[32];
    bool staEnable;
    char staSSID[32];
    char staPassword[32];
} WifiConnectionSettings;

class WifiConnection
{
public:
    WifiConnectionSettings settings;
    WebServer *ws;
    Preferences *preferences;
    const char *preferencesNS;
    MPU *mpu;
    const char *html =
#include "html.h"
        ;

    void setup(Preferences *p, MPU *m, const char *preferencesNS = "WiFi")
    {
        preferences = p;
        this->preferencesNS = preferencesNS;
        mpu = m;
        loadSettings();
        //settings.staEnable = true;
        //strncpy(settings.staSSID, "ssid", 32);
        //strncpy(settings.staPassword, "pw", 32);
        //saveSettings();
        applySettings();
        ws = new WebServer();
        ws->on("/", [this] { handleRoot(); });
        ws->on("/calibrateAccelGyro", [this] { handleCalibrateAccelGyro(); });
        ws->on("/calibrateMag", [this] { handleCalibrateMag(); });
        ws->on("/reboot", [this] { handleReboot(); });
        ws->onNotFound([this]() { handle404(); });
        ws->begin();
    }

    void loop(const ulong t)
    {
        ws->handleClient();
    }

    void loadSettings()
    {
        if (!preferences->begin(preferencesNS, true)) {      // try ro mode
            if (!preferences->begin(preferencesNS, false)) { // open in rw mode to create ns
                log_e("Preferences begin failed for '%s'.", preferencesNS);
                while (1)
                    ;
            }
        }
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
        preferences->end();
    }

    void saveSettings()
    {
        log_d("Saving settings to %s", preferencesNS);
        if (!preferences->begin(preferencesNS, false)) {
            log_e("Preferences begin failed for '%s'.", preferencesNS);
            return;
        }
        preferences->putBool("apEnable", settings.apEnable);
        preferences->putString("apSSID", settings.apSSID);
        preferences->putString("apPassword", settings.apPassword);
        preferences->putBool("staEnable", settings.staEnable);
        preferences->putString("staSSID", settings.staSSID);
        preferences->putString("staPassword", settings.staPassword);
        preferences->end();
        log_d("Settings saved");
    }

    void printSettings()
    {
        Serial.printf("AP %s '%s' '%s', STA %s '%s' '%s'",
                      settings.apEnable ? "Enabled" : "Disabled",
                      settings.apSSID,
                      settings.apPassword,
                      settings.staEnable ? "Enabled" : "Disabled",
                      settings.staSSID,
                      settings.staPassword);
    }

    void applySettings()
    {
        if (settings.apEnable || settings.staEnable) {
            if (settings.apEnable && settings.staEnable)
                WiFi.mode(WIFI_MODE_APSTA);
            else if (settings.apEnable)
                WiFi.mode(WIFI_MODE_AP);
            else if (settings.staEnable)
                WiFi.mode(WIFI_MODE_STA);

        } else
            WiFi.mode(WIFI_MODE_NULL);
        if (settings.apEnable) {
            WiFi.softAP(settings.apSSID, settings.apPassword);
        }
        if (settings.staEnable) {
            WiFi.begin(settings.staSSID, settings.staPassword);
            Serial.print("Connecting to WiFi");
            while (WiFi.status() != WL_CONNECTED) {
                Serial.print(".");
                delay(300);
            }
            Serial.printf("connected\nLocal IP: %s\n", WiFi.localIP().toString().c_str());
        }
    }

    bool connected()
    {
        //return WiFi.softAPgetStationNum() > 0;
        return WiFi.isConnected();
    }

    void handleRoot()
    {
        ws->send(200, "text/html", html);
    }

    void handleCalibrateAccelGyro()
    {
        mpu->calibrateAccelGyro();
        ws->send(200, "text/plain", "Accel/Gyro calibrated.");
    }

    void handleCalibrateMag()
    {
        mpu->calibrateMag();
        ws->send(200, "text/plain", "Magnetometer calibrated.");
    }

    void handleReboot()
    {
        Serial.println("Rebooting...");
        ESP.restart();
    }

    void handle404()
    {
        Serial.println(ws->uri().c_str());
        ws->sendHeader("Location", "/", true);
        ws->send(302, "text/plain", "302 Moved");
    }
};

#endif