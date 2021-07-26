#include "status.h"
#include "board.h"

#include "battery.h"
#include "mpu.h"
#include "power.h"
#include "strain.h"
#include "wificonnection.h"

void Status::setup() {
    statusEnabled = true;
#ifdef COMPILE_TIMESTAMP
    log_i("Compile timestamp %d\n", COMPILE_TIMESTAMP);
#endif
#ifdef COMPILE_TIMESTRING
    log_i("Compile timestring %s\n", COMPILE_TIMESTRING);
#endif
    printHeader();
}

void Status::loop() {
    if (0 < Serial.available()) {
        handleInput(getChar());
    }
    print();
}

char Status::getChar() {
    bool oldStatusEnabled = statusEnabled;
    statusEnabled = false;
    while (!Serial.available()) {
        vTaskDelay(10);
    }
    statusEnabled = oldStatusEnabled;
    return Serial.read();
}

int Status::getStr(char *str, int maxLength) { return getStr(str, maxLength, true); }
int Status::getStr(char *str, int maxLength, bool echo) {
    bool oldStatusEnabled = statusEnabled;
    statusEnabled = false;
    int received = -1;
    char buffer[maxLength];
    char c;
    while (1) {
        c = getChar();
        received++;
        if ('\n' == c) {
            break;
        }
        if ('\r' == c) {
            received--;
            continue;
        }
        if (received >= maxLength) {
            break;
        }
        if (echo) {
            Serial.print(c);
        }
        buffer[received] = c;
    }
    buffer[received] = '\0';
    strncpy(str, buffer, maxLength);
    Serial.print("\n");
    statusEnabled = oldStatusEnabled;
    return received;
}

void Status::setFreq(float freq) {
    if (freq < 0.1 || (float)taskFreq < freq) {
        Serial.printf("Frequency %.2f out of range\n", freq);
        return;
    }
    statusDelay = (uint32_t)(1000.0 / freq);
    Serial.printf("Status freq is %.2fHz (%dms delay)\n", freq, statusDelay);
}

void Status::printHeader() {
    Serial.println("[Status] [Key] Rpm Strain Power Voltage Sleep");
}

void Status::print() {
    const ulong t = millis();
    static ulong lastOutput = 0;
    if (!statusEnabled)
        return;
    if (lastOutput < t - statusDelay) {
        if (BOOTMODE_OTA == board.lastBootMode)
            Serial.println("Waiting for OTA");
        else
            Serial.printf(
                "[Status] %d %d %d %.2f %.2f\n",
                (int)board.getRpm(),
                (int)board.getLiveStrain(),
                (int)board.getPower(),  // not emptying the buffer
                board.battery.voltage,
                board.timeUntilDeepSleep(t) / 60000.0);  // time in minutes
        lastOutput = t;
    }
}

/*
        [c]alibrate 
            [a]ccel/gyro
            [m]ag
            [b]attery
            [s]train
            set [c]rank length
            toggle [r]everse strain
            toggle r[e]verse mpu
            toggle [d]ouble power
            [p]rint calibration
            e[x]it
        [w]ifi
            [a]p
                [e]nable
                [s]sid
                [p]assword
                e[x]it
            [s]ta
                [e]nable
                [s]sid
                [p]assword
                e[x]it
            [p]rint config
            e[x]it
        [b]le
            toggle c[a]dence in cpm
            toggle [c]sc service
            [p]rint config
            e[x]it
        c[o]nfig
            [h]ostname
            status [f]requency
            sleep [d]elay
        [l]ive mode
        [r]eboot
        [d]eep sleep
    */
void Status::handleInput(const char input) {
    char tmpStr[32] = "";
    float tmpF = 0.0;
    char menu[8];
    strncpy(menu, &input, 1);
    menu[1] = '\0';
    while (1) {
        switch (menu[0]) {
            case 'c':  // calibrate
                switch (menu[1]) {
                    case 'a':
                        board.mpu.calibrateAccelGyro();
                        board.mpu.saveCalibration();
                        board.mpu.printAccelGyroCalibration();
                        menu[1] = '\0';
                        break;
                    case 'm':
                        board.mpu.calibrateMag();
                        board.mpu.saveCalibration();
                        board.mpu.printMagCalibration();
                        menu[1] = '\0';
                        break;
                    case 'b':
                        board.battery.printCalibration();
                        Serial.print("Enter measured battery voltage and press [Enter]: ");
                        getStr(tmpStr, sizeof tmpStr);
                        board.battery.calibrateTo(atof(tmpStr));
                        board.battery.saveCalibration();
                        board.battery.printCalibration();
                        menu[1] = '\0';
                        break;
                    case 's':
                        board.strain.printCalibration();
                        Serial.print("Enter known mass in Kg and press [Enter]: ");
                        getStr(tmpStr, sizeof tmpStr);
                        tmpF = atof(tmpStr);
                        if (1 < tmpF && tmpF < 1000) {
                            board.strain.calibrateTo(tmpF);
                            board.strain.saveCalibration();
                        } else
                            Serial.println("Invalid value.");
                        board.strain.printCalibration();
                        menu[1] = '\0';
                        break;
                    case 'c':
                        Serial.print("Enter crank length in mm and press [Enter]: ");
                        getStr(tmpStr, sizeof tmpStr);
                        tmpF = atof(tmpStr);
                        if (10 < tmpF && tmpF < 1000) {
                            board.power.crankLength = tmpF;
                            board.power.saveSettings();
                        } else
                            Serial.println("Invalid value.");
                        menu[1] = '\0';
                        break;
                    case 'r':
                        board.power.reverseStrain = !board.power.reverseStrain;
                        board.power.saveSettings();
                        Serial.printf("Strain is now %sreversed\n", board.power.reverseStrain ? "" : "not ");
                        menu[1] = '\0';
                        break;
                    case 'e':
                        board.power.reverseMPU = !board.power.reverseMPU;
                        board.power.saveSettings();
                        Serial.printf("MPU is now %sreversed\n", board.power.reverseMPU ? "" : "not ");
                        menu[1] = '\0';
                        break;
                    case 'd':
                        board.power.reportDouble = !board.power.reportDouble;
                        board.power.saveSettings();
                        Serial.printf("Power is now %sdoubled\n", board.power.reportDouble ? "" : "not ");
                        menu[1] = '\0';
                        break;
                    case 'p':
                        board.mpu.printCalibration();
                        board.strain.printCalibration();
                        board.battery.printCalibration();
                        board.power.printSettings();
                        menu[1] = '\0';
                        break;
                    case 'x':
                        strncpy(menu, " ", 2);
                        break;
                    default:
                        Serial.print("Calibrate [a]ccel/gyro, [m]ag, [b]attery, [s]train, set [c]rank length, toggle [r]everse strain, toggle r[e]verse MPU, toggle [d]ouble power, [p]rint calibration or e[x]it\n");
                        menu[1] = getChar();
                        menu[2] = '\0';
                }
                break;
            case 'w':  // wifi
                switch (menu[1]) {
                    case 'a':
                        switch (menu[2]) {
                            case 'e':
                                board.wifi.settings.apEnable = !board.wifi.settings.apEnable;
                                board.wifi.applySettings();
                                board.wifi.saveSettings();
                                menu[2] = '\0';
                                break;
                            case 's':
                                Serial.print("Enter AP SSID (max 31 chars) and press [Enter]: ");
                                getStr(board.wifi.settings.apSSID, 32);
                                board.wifi.applySettings();
                                board.wifi.saveSettings();
                                menu[2] = '\0';
                                break;
                            case 'p':
                                Serial.print("Enter AP password (max 31 chars) and press [Enter]: ");
                                getStr(board.wifi.settings.apPassword, 32, false);
                                board.wifi.applySettings();
                                board.wifi.saveSettings();
                                menu[2] = '\0';
                                break;
                            case 'x':
                                strncpy(menu, "w ", 3);
                                break;
                            default:
                                board.wifi.printSettings();
                                Serial.print("AP setup: [e]nable, [s]SID, [p]assword or e[x]it\n");
                                menu[2] = getChar();
                                menu[3] = '\0';
                        }
                        break;
                    case 's':
                        switch (menu[2]) {
                            case 'e':
                                board.wifi.settings.staEnable = !board.wifi.settings.staEnable;
                                board.wifi.applySettings();
                                board.wifi.saveSettings();
                                menu[2] = '\0';
                                break;
                            case 's':
                                Serial.print("Enter STA SSID (max 31 chars) and press [Enter]: ");
                                getStr(board.wifi.settings.staSSID, 32);
                                board.wifi.applySettings();
                                board.wifi.saveSettings();
                                menu[2] = '\0';
                                break;
                            case 'p':
                                Serial.print("Enter STA password (max 31 chars) and press [Enter]: ");
                                getStr(board.wifi.settings.staPassword, 32, false);
                                board.wifi.applySettings();
                                board.wifi.saveSettings();
                                menu[2] = '\0';
                                break;
                            case 'x':
                                strncpy(menu, "w ", 3);
                                break;
                            default:
                                board.wifi.printSettings();
                                Serial.print("STA setup: [e]nable, [s]SID, [p]assword or e[x]it\n");
                                menu[2] = getChar();
                                menu[3] = '\0';
                        }
                        break;
                    case 'p':
                        board.wifi.printSettings();
                        menu[1] = '\0';
                        break;
                    case 'x':
                        strncpy(menu, " ", 2);
                        break;
                    default:
                        Serial.print("WiFi setup: [a]P, [s]TA, [p]rint config or e[x]it\n");
                        menu[1] = getChar();
                        menu[2] = '\0';
                }
                break;
            case 'o':  // config
                switch (menu[1]) {
                    case 'h':
                        Serial.print("Enter hostname (no spaces, no special chars, max 31) and press [Enter]: ");
                        getStr(tmpStr, 32);
                        if (1 < strlen(tmpStr))
                            strncpy(board.hostName, tmpStr, 32);
                        board.saveSettings();
                        menu[1] = '\0';
                        break;
                    case 'f':
                        Serial.printf("Enter status output frequency in Hz (0.1...%d) and press [Enter]: ", taskFreq);
                        getStr(tmpStr, 32);
                        setFreq((float)atof(tmpStr));
                        // status freq is not saved
                        menu[1] = '\0';
                        break;
                    case 'd':
                        Serial.print("Enter deep sleep delay in minutes and press [Enter]: ");
                        getStr(tmpStr, 32);
                        board.setSleepDelay(atof(tmpStr) * 60 * 1000);
                        // sleep delay is not saved
                        menu[1] = '\0';
                        break;
                    case 'x':
                        strncpy(menu, " ", 2);
                        break;
                    default:
                        Serial.print("Config: [h]ostname, status [f]requency, sleep [d]elay or e[x]it\n");
                        menu[1] = getChar();
                        menu[2] = '\0';
                }
                break;
            case 'b':  // ble
                switch (menu[1]) {
                    case 'a':
                        board.ble.setCadenceInCpm(!board.ble.cadenceInCpm);
                        menu[1] = '\0';
                        break;
                    case 'c':
                        board.ble.setCscServiceActive(!board.ble.cscServiceActive);
                        menu[1] = '\0';
                        break;
                    case 'x':
                        strncpy(menu, " ", 2);
                        break;
                    default:
                        board.ble.printSettings();
                        Serial.print("BLE: toggle c[a]dence in cpm, toggle [c]sc service, [p]rint config or e[x]it\n");
                        menu[1] = getChar();
                        menu[2] = '\0';
                }
                break;
            case 'l':
                Serial.print("Entering live mode\n");
                Serial.flush();
#ifdef FEATURE_WEBSERVER
                board.webserver.off();
#endif
#ifdef FEATURE_SERIAL
                board.wifiSerial.off();
#endif
                board.ota.off();
                board.wifi.off();
                return;
            case 'r':
                Serial.print("Rebooting...\n");
                Serial.flush();
                ESP.restart();
                return;
            case 'd':
                board.deepSleep();
                return;
            case NULL:
                Serial.print("NULL received\n");
                return;
            case 4:  // EOT
                //Serial.print("Ctrl+D received\n");
                return;
            default:
                Serial.print("[c]alibrate, [w]iFi, [b]le, c[o]nfig, [l]ive mode, [r]eboot or [d]eep sleep\n");
                return;
        }
    }
}
