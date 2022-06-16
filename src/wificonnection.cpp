#include "wificonnection.h"
#include "board.h"

void WifiConnection::setup(Preferences *p, const char *preferencesNS) {
    preferencesSetup(p, preferencesNS);
    loadDefaultSettings();
    loadSettings();
    applySettings();
    registerCallbacks();
};

void WifiConnection::loop(){};

void WifiConnection::off() {
    Serial.println("[Wifi] Shutting down");
    settings.enabled = false;
    applySettings();
    taskStop();
};

void WifiConnection::loadSettings() {
    if (!preferencesStartLoad()) return;
    settings.enabled = preferences->getBool("enabled", settings.enabled);
    settings.apEnabled = preferences->getBool("apEnabled", settings.apEnabled);
    // preferences->getBytes("apSSID", settings.apSSID, 32);
    // preferences->getBytes("apPassword", settings.apPassword, 32);
    strncpy(settings.apSSID, preferences->getString("apSSID", settings.apSSID).c_str(), 32);
    strncpy(settings.apPassword, preferences->getString("apPassword", settings.apPassword).c_str(), 32);
    settings.staEnabled = preferences->getBool("staEnabled", settings.staEnabled);
    // preferences->getBytes("staSSID", settings.staSSID, 32);
    // preferences->getBytes("staPassword", settings.staPassword, 32);
    strncpy(settings.staSSID, preferences->getString("staSSID", settings.staSSID).c_str(), 32);
    strncpy(settings.staPassword, preferences->getString("staPassword", settings.staPassword).c_str(), 32);
    preferencesEnd();
};

void WifiConnection::loadDefaultSettings() {
    settings.enabled = true;
    settings.apEnabled = true;
    strncpy(settings.apSSID, HOSTNAME, 32);
    strncpy(settings.apPassword, "", 32);
    settings.staEnabled = false;
    strncpy(settings.staSSID, "", 32);
    strncpy(settings.staPassword, "", 32);
};

void WifiConnection::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putBool("enabled", settings.enabled);
    preferences->putBool("apEnabled", settings.apEnabled);
    preferences->putString("apSSID", settings.apSSID);
    preferences->putString("apPassword", settings.apPassword);
    preferences->putBool("staEnabled", settings.staEnabled);
    preferences->putString("staSSID", settings.staSSID);
    preferences->putString("staPassword", settings.staPassword);
    preferencesEnd();
};

void WifiConnection::printSettings() {
    Serial.printf("[Wifi] Wifi %sabled\n",
                  settings.enabled ? "En" : "Dis");
    printAPSettings();
    printSTASettings();
};

void WifiConnection::printAPSettings() {
    Serial.printf("[Wifi] AP %sabled '%s' '%s'\n",
                  settings.apEnabled ? "En" : "Dis",
                  settings.apSSID,
                  "***"  // settings.apPassword
    );
    if (settings.apEnabled)
        Serial.printf("[Wifi] AP online, IP: %s\n", WiFi.softAPIP().toString().c_str());
};

void WifiConnection::printSTASettings() {
    Serial.printf("[Wifi] STA %sabled '%s' '%s'\n",
                  settings.staEnabled ? "En" : "Dis",
                  settings.staSSID,
                  "***"  // settings.staPassword
    );
    if (WiFi.isConnected())
        Serial.printf("[Wifi] STA connected, local IP: %s\n", WiFi.localIP().toString().c_str());
};

void WifiConnection::applySettings() {
    Serial.println("[Wifi] Applying settings, connections will be reset");
    Serial.flush();
    delay(1000);
    wifi_mode_t oldWifiMode = WiFi.getMode();
    wifi_mode_t newWifiMode;
    if (settings.enabled && (settings.apEnabled || settings.staEnabled)) {
        if (settings.apEnabled && settings.staEnabled)
            newWifiMode = WIFI_MODE_APSTA;
        else if (settings.apEnabled)
            newWifiMode = WIFI_MODE_AP;
        else
            newWifiMode = WIFI_MODE_STA;
    } else
        newWifiMode = WIFI_MODE_NULL;
    WiFi.mode(newWifiMode);
    if (settings.enabled && settings.apEnabled) {
        if (0 == strcmp("", const_cast<char *>(settings.apSSID))) {
            log_e("Warning: cannot enable AP with empty SSID");
            settings.apEnabled = false;
        } else {
            log_i("Setting up WiFi AP '%s'", settings.apSSID);
            WiFi.softAP(settings.apSSID, settings.apPassword);
        }
    }
    if (settings.enabled && settings.staEnabled) {
        if (0 == strcmp("", const_cast<char *>(settings.staSSID))) {
            log_e("Warning: cannot enable STA with empty SSID");
            settings.staEnabled = false;
        } else {
            log_i("Connecting WiFi STA to AP '%s'", settings.staSSID);
            WiFi.begin(settings.staSSID, settings.staPassword);
        }
    }
    if (oldWifiMode != newWifiMode) {
        if (newWifiMode == WIFI_MODE_NULL) {
            board.stopTask("ota");
            board.stopTask("wifiSerial");
        } else {
            board.stopTask("ota");
            board.ota.setup(board.hostName);
            board.startTask("ota");
#ifdef FEATURE_SERIAL
            board.stopTask("wifiSerial");
            board.wifiSerial.setup();
            board.startTask("wifiSerial");
#endif
        }
    }
};

void WifiConnection::setEnabled(bool state) {
    settings.enabled = state;
    applySettings();
    saveSettings();
};

bool WifiConnection::isEnabled() {
    return settings.enabled;
};

bool WifiConnection::connected() {
    return WiFi.isConnected() || WiFi.softAPgetStationNum() > 0;
};

void WifiConnection::registerCallbacks() {
    WiFi.onEvent(
        [this](arduino_event_id_t event, arduino_event_info_t info) {
            onEvent(event, info);
        },
        ARDUINO_EVENT_MAX);
}

void WifiConnection::onEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            log_i("[AP] station connected, now active: %d", WiFi.softAPgetStationNum());
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            log_i("[AP] station got IP");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            log_i("[AP] station disconnected, now active: %d", WiFi.softAPgetStationNum());
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            log_i("[STA] starting");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            log_i("[STA] connected");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            log_i("[STA] got IP: %s", WiFi.localIP().toString().c_str());
            // WiFi.setAutoReconnect(true);
            // WiFi.persistent(true);
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            log_i("[STA] disconnected");
            if (settings.enabled && settings.staEnabled) {
                log_i("[STA] reconnecting");
                // WiFi.disconnect();
                WiFi.reconnect();
                // WiFi.begin();
            }
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            log_i("[STA] stopped");
            break;
        default:
            log_i("event: %d, info: %d", event, info);
    }
}
