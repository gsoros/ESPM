#ifndef SERIALIO_H
#define SERIALIO_H

#include <Arduino.h>

#include "battery.h"
#include "mpu.h"
#include "strain.h"
#include "wificonnection.h"

class SerialIO {
   public:
    Battery *battery;
    MPU *mpu;
    Strain *strain;
    WifiConnection *wifi;
    bool statusEnabled = false;

    void setup(Battery *b, MPU *m, Strain *s, WifiConnection *w) {
        battery = b;
        mpu = m;
        strain = s;
        wifi = w;
        Serial.begin(115200);
        while (!Serial)
            delay(1);
        statusEnabled = true;
#ifdef COMPILE_TIMESTAMP
        log_i("Compile timestamp %d\n", COMPILE_TIMESTAMP);
#endif
#ifdef COMPILE_TIMESTRING
        log_i("Compile timestring %s\n", COMPILE_TIMESTRING);
#endif
    }

    void loop(const ulong t) {
        if (Serial.available() > 0) {
            handleInput(getChar());
        }
        printStatus(t);
    }

    char getChar() {
        while (!Serial.available()) {
            delay(10);
        }
        return Serial.read();
    }

    int getStr(char *str, int maxLength, bool echo = true) {
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
        Serial.printf("\n");
        return received;
    }

    void printStatus(const ulong t) {
        static ulong lastOutput = 0;
        if (!statusEnabled)
            return;
        if (lastOutput < t - 2000) {
            Serial.printf(
                //"%f %f %d %d\n",
                "%f %f %f\n",
                //((int)t)%100,
                mpu->measurement,
                strain->measurement,
                battery->voltage
                //mpu->idleCyclesMax,
                //strain->idleCyclesMax
            );
            //mpu->idleCyclesMax = 0;
            //strain->idleCyclesMax = 0;
            lastOutput = t;
        }
    }

    /*
        [c]alibrate 
            [a]ccel/gyro
            [m]ag
            [b]attery
            [s]train
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
        [r]eboot
    */
    void handleInput(const char input) {
        char menu[8];
        strncpy(menu, &input, 1);
        menu[1] = '\0';
        while (1) {
            switch (menu[0]) {
                case 'c':
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
                            Serial.printf("Enter measured battery voltage and press [Enter]: ");
                            char voltage[32];
                            getStr(voltage, 32);
                            battery->calibrateTo(atof(voltage));
                            menu[1] = '\0';
                            break;
                        case 's':
                            strain->printCalibration();
                            Serial.printf("Enter known mass and press [Enter]: ");
                            char mass[32];
                            getStr(mass, 32);
                            strain->calibrateTo(atof(mass));
                            strain->saveCalibration();
                            strain->printCalibration();
                            menu[1] = '\0';
                            break;
                        case 'x':
                            strncpy(menu, " ", 2);
                            break;
                        default:
                            printf("Calibrate [a]ccel/gyro, [m]ag, [b]attery, [s]train or e[x]it\n");
                            menu[1] = getChar();
                            menu[2] = '\0';
                    }
                    break;
                case 'w':
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
                                    Serial.printf("Enter AP SSID (max 31 chars) and press [Enter]: ");
                                    getStr(wifi->settings.apSSID, 32);
                                    wifi->applySettings();
                                    wifi->saveSettings();
                                    menu[2] = '\0';
                                    break;
                                case 'p':
                                    Serial.printf("Enter AP password (max 31 chars) and press [Enter]: ");
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
                                    printf("AP setup: [e]nable, [s]SID, [p]assword or e[x]it\n");
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
                                    Serial.printf("Enter STA SSID (max 31 chars) and press [Enter]: ");
                                    getStr(wifi->settings.staSSID, 32);
                                    wifi->applySettings();
                                    wifi->saveSettings();
                                    menu[2] = '\0';
                                    break;
                                case 'p':
                                    Serial.printf("Enter STA password (max 31 chars) and press [Enter]: ");
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
                                    printf("STA setup: [e]nable, [s]SID, [p]assword or e[x]it\n");
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
                            printf("WiFi setup: [a]P, [s]TA, [p]rint config or e[x]it\n");
                            menu[1] = getChar();
                            menu[2] = '\0';
                    }
                    break;
                case 'r':
                    ESP.restart();
                    menu[0] = '\0';
                    break;
                default:
                    printf("[c]alibrate, [w]iFi or [r]eboot\n");
                    menu[0] = '\0';
                    return;
            }
        }
    }
};

#endif