#ifndef STATUS_H
#define STATUS_H

#include <Arduino.h>

#include "battery.h"
#include "mpu.h"
#include "power.h"
#include "strain.h"
#include "task.h"
#include "wificonnection.h"

class Status : public Task {
   public:
    Battery *battery;
    MPU *mpu;
    Strain *strain;
    Power *power;
    WifiConnection *wifi;
    bool statusEnabled = false;
    uint32_t statusDelay = 1000;  // ms

    void setup(
        Battery *b,
        MPU *m,
        Strain *s,
        Power *p,
        WifiConnection *w) {
        battery = b;
        mpu = m;
        strain = s;
        power = p;
        wifi = w;
        statusEnabled = true;
#ifdef COMPILE_TIMESTAMP
        log_i("Compile timestamp %d\n", COMPILE_TIMESTAMP);
#endif
#ifdef COMPILE_TIMESTRING
        log_i("Compile timestring %s\n", COMPILE_TIMESTRING);
#endif
    }

    void loop(const ulong t) {
        if (0 < Serial.available()) {
            handleInput(getChar());
        }
        printStatus(t);
    }

    char getChar() {
        bool oldStatusEnabled = statusEnabled;
        statusEnabled = false;
        while (!Serial.available()) {
            vTaskDelay(10);
        }
        statusEnabled = oldStatusEnabled;
        return Serial.read();
    }

    int getStr(char *str, int maxLength, bool echo = true) {
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

    void setStatusFreq(float freq) {
        if (freq < 0.1 || (float)taskFreq < freq) {
            Serial.printf("Frequency %.2f out of range\n", freq);
            return;
        }
        statusDelay = (uint32_t)(1000.0 / freq);
        Serial.printf("Status freq is %.2fHz (%dms delay)\n", freq, statusDelay);
    }

    void printStatus(const ulong t) {
        static ulong lastOutput = 0;
        if (!statusEnabled)
            return;
        if (lastOutput < t - statusDelay) {
            Serial.printf(
                "%d %d %d %.2f %.2f\n",
                (int)mpu->rpm(),
                (int)strain->measurement(),
                (int)power->power(),
                battery->pinVoltage,
                battery->voltage);
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
            toggle r[e]verse MPU
            [p]rint calibrations
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
        c[o]nfig
            status [f]requency
        [r]eboot
    */
    void handleInput(const char input) {
        char tmpStr[32];
        char menu[8];
        strncpy(menu, &input, 1);
        menu[1] = '\0';
        while (1) {
            switch (menu[0]) {
                case 'c':  // calibrate
                    switch (menu[1]) {
                        case 'a':
                            mpu->calibrateAccelGyro();
                            mpu->saveCalibration();
                            mpu->printAccelGyroCalibration();
                            menu[1] = '\0';
                            break;
                        case 'm':
                            mpu->calibrateMag();
                            mpu->saveCalibration();
                            mpu->printMagCalibration();
                            menu[1] = '\0';
                            break;
                        case 'b':
                            battery->printCalibration();
                            Serial.print("Enter measured battery voltage and press [Enter]: ");
                            getStr(tmpStr, sizeof tmpStr);
                            battery->calibrateTo(atof(tmpStr));
                            battery->saveCalibration();
                            battery->printCalibration();
                            menu[1] = '\0';
                            break;
                        case 's':
                            strain->printCalibration();
                            Serial.print("Enter known mass in Kg and press [Enter]: ");
                            getStr(tmpStr, sizeof tmpStr);
                            strain->calibrateTo(atof(tmpStr));
                            strain->saveCalibration();
                            strain->printCalibration();
                            menu[1] = '\0';
                            break;
                        case 'c':
                            Serial.print("Enter crank length in mm and press [Enter]: ");
                            getStr(tmpStr, sizeof tmpStr);
                            power->crankLength = atof(tmpStr);
                            power->saveSettings();
                            menu[1] = '\0';
                            break;
                        case 'r':
                            power->reverseStrain = !power->reverseStrain;
                            power->saveSettings();
                            Serial.printf("Strain is now %sreversed\n", power->reverseStrain ? "" : "not ");
                            menu[1] = '\0';
                            break;
                        case 'e':
                            power->reverseMPU = !power->reverseMPU;
                            power->saveSettings();
                            Serial.printf("MPU is now %sreversed\n", power->reverseMPU ? "" : "not ");
                            menu[1] = '\0';
                            break;
                        case 'p':
                            mpu->printCalibration();
                            strain->printCalibration();
                            battery->printCalibration();
                            power->printSettings();
                            menu[1] = '\0';
                            break;
                        case 'x':
                            strncpy(menu, " ", 2);
                            break;
                        default:
                            Serial.print("Calibrate [a]ccel/gyro, [m]ag, [b]attery, [s]train, set [c]rank length, toggle [r]everse strain, toggle r[e]verse MPU, [p]rint calibration or e[x]it\n");
                            menu[1] = getChar();
                            menu[2] = '\0';
                    }
                    break;
                case 'w':  // wifi
                    switch (menu[1]) {
                        case 'a':
                            switch (menu[2]) {
                                case 'e':
                                    wifi->settings.apEnable = !wifi->settings.apEnable;
                                    wifi->applySettings();
                                    wifi->saveSettings();
                                    menu[2] = '\0';
                                    break;
                                case 's':
                                    Serial.print("Enter AP SSID (max 31 chars) and press [Enter]: ");
                                    getStr(wifi->settings.apSSID, 32);
                                    wifi->applySettings();
                                    wifi->saveSettings();
                                    menu[2] = '\0';
                                    break;
                                case 'p':
                                    Serial.print("Enter AP password (max 31 chars) and press [Enter]: ");
                                    getStr(wifi->settings.apPassword, 32, false);
                                    wifi->applySettings();
                                    wifi->saveSettings();
                                    menu[2] = '\0';
                                    break;
                                case 'x':
                                    strncpy(menu, "w ", 3);
                                    break;
                                default:
                                    wifi->printSettings();
                                    Serial.print("AP setup: [e]nable, [s]SID, [p]assword or e[x]it\n");
                                    menu[2] = getChar();
                                    menu[3] = '\0';
                            }
                            break;
                        case 's':
                            switch (menu[2]) {
                                case 'e':
                                    wifi->settings.staEnable = !wifi->settings.staEnable;
                                    wifi->applySettings();
                                    wifi->saveSettings();
                                    menu[2] = '\0';
                                    break;
                                case 's':
                                    Serial.print("Enter STA SSID (max 31 chars) and press [Enter]: ");
                                    getStr(wifi->settings.staSSID, 32);
                                    wifi->applySettings();
                                    wifi->saveSettings();
                                    menu[2] = '\0';
                                    break;
                                case 'p':
                                    Serial.print("Enter STA password (max 31 chars) and press [Enter]: ");
                                    getStr(wifi->settings.staPassword, 32, false);
                                    wifi->applySettings();
                                    wifi->saveSettings();
                                    menu[2] = '\0';
                                    break;
                                case 'x':
                                    strncpy(menu, "w ", 3);
                                    break;
                                default:
                                    wifi->printSettings();
                                    Serial.print("STA setup: [e]nable, [s]SID, [p]assword or e[x]it\n");
                                    menu[2] = getChar();
                                    menu[3] = '\0';
                            }
                            break;
                        case 'p':
                            wifi->printSettings();
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
                        case 'f':
                            Serial.printf("Enter status output frequency in Hz (0.1...%d) and press [Enter]: ", taskFreq);
                            getStr(tmpStr, 32);
                            setStatusFreq((float)atof(tmpStr));
                            // TODO save settings
                            menu[1] = '\0';
                            break;
                        case 'x':
                            strncpy(menu, " ", 2);
                            break;
                        default:
                            Serial.print("Config: status [f]requency or e[x]it\n");
                            menu[1] = getChar();
                            menu[2] = '\0';
                    }
                    break;
                case 'r':
                    Serial.print("Rebooting...\n");
                    Serial.flush();
                    ESP.restart();
                    return;
                case NULL:
                    Serial.print("NULL received\n");
                    return;
                case 4:
                    //Serial.print("Ctrl-D received\n");
                    return;
                default:
                    Serial.print("[c]alibrate, [w]iFi, c[o]nfig or [r]eboot\n");
                    return;
            }
        }
    }
};

#endif