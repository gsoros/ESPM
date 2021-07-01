#ifndef SERIALIO_H
#define SERIALIO_H

#include <Arduino.h>
#include <Stream.h>

#include "battery.h"
#include "mpu.h"
#include "power.h"
#include "strain.h"
#include "task.h"
#include "wificonnection.h"

class SerialIO : public Task {
   public:
    Stream *s0;
    bool s0_enabled;
    Stream *s1;
    bool s1_enabled;
    Battery *battery;
    MPU *mpu;
    Strain *strain;
    Power *power;
    WifiConnection *wifi;
    bool statusEnabled = false;

    void setup(
        Stream *stream0,
        Stream *stream1,
        Battery *b,
        MPU *m,
        Strain *s,
        Power *p,
        WifiConnection *w,
        bool stream0_enabled = true,
        bool stream1_enabled = false) {
        s0 = stream0;
        s0_enabled = stream0_enabled;
        s1 = stream1;
        s1_enabled = stream1_enabled;
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
        if (0 < available()) {
            handleInput(getChar());
        }
        printStatus(t);
    }

    int available() {
        return (s0_enabled ? 0 < s0->available() : 0) + (s1_enabled ? 0 < s1->available() : 0);
    }

    int read() {
        return s0_enabled && s0->available()   ? s0->read()
               : s1_enabled && s1->available() ? s1->read()
                                               : -1;
    }

    char getChar() {
        bool oldStatusEnabled = statusEnabled;
        statusEnabled = false;
        while (!available()) {
            vTaskDelay(10);
        }
        statusEnabled = oldStatusEnabled;
        return read();
    }

    size_t print(const char c) {
        int len0 = 0, len1 = 0;
        if (s0_enabled) len0 = s0->print(c);
        if (s1_enabled) len1 = s1->print(c);
        return max(len0, len1);
    }
    size_t print(const char *str) {
        int len0 = 0, len1 = 0;
        if (s0_enabled) len0 = s0->print(str);
        if (s1_enabled) len1 = s1->print(str);
        return max(len0, len1);
    }

    size_t printf(const char *format, ...) {
        va_list args;
        size_t ret;
        va_start(args, format);
        ret = vprintf(format, args);
        va_end(args);
        return ret;
    }

    size_t vprintf(const char *format, va_list arg) {
        char buf[64];
        vsnprintf(buf, sizeof(buf), format, arg);
        int len0 = 0, len1 = 0;
        if (s0_enabled) len0 = s0->print(buf);
        if (s1_enabled) len1 = s1->printf(buf);
        return max(len0, len1);
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
                print(c);
            }
            buffer[received] = c;
        }
        buffer[received] = '\0';
        strncpy(str, buffer, maxLength);
        printf("\n");
        statusEnabled = oldStatusEnabled;
        return received;
    }

    void printStatus(const ulong t) {
        static ulong lastOutput = 0;
        if (!statusEnabled)
            return;
        if (lastOutput < t - 100) {
            printf(
                //"%f %f %d %d\n",
                "%f %f %f %.2f %.2f %d %d %d %d\n",
                mpu->rpm(),
                strain->measurement(),
                power->power,
                battery->pinVoltage,
                battery->voltage,
                //(int)battery->taskLastLoopDelay,
                (int)strain->taskLastLoopDelay,
                (int)strain->lastMeasurementDelay,
                ESP.getMinFreeHeap(),
                ((int)t) % 100000
                //mpu->idleCyclesMax,
                //strain->idleCyclesMax
            );
            //s1->printf("%f\n", mpu->rpm());
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
        [r]eboot
    */
    void handleInput(const char input) {
        char tmpStr[32];
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
                            battery->printCalibration();
                            print("Enter measured battery voltage and press [Enter]: ");
                            getStr(tmpStr, sizeof tmpStr);
                            battery->calibrateTo(atof(tmpStr));
                            battery->saveCalibration();
                            battery->printCalibration();
                            menu[1] = '\0';
                            break;
                        case 's':
                            strain->printCalibration();
                            print("Enter known mass in Kg and press [Enter]: ");
                            getStr(tmpStr, sizeof tmpStr);
                            strain->calibrateTo(atof(tmpStr));
                            strain->saveCalibration();
                            strain->printCalibration();
                            menu[1] = '\0';
                            break;
                        case 'c':
                            print("Enter crank length in mm and press [Enter]: ");
                            getStr(tmpStr, sizeof tmpStr);
                            power->crankLength = atof(tmpStr);
                            power->saveSettings();
                            menu[1] = '\0';
                            break;
                        case 'r':
                            power->reverseStrain = !power->reverseStrain;
                            power->saveSettings();
                            printf("Strain is now %sreversed\n", power->reverseStrain ? "" : "not ");
                            menu[1] = '\0';
                            break;
                        case 'e':
                            power->reverseMPU = !power->reverseMPU;
                            power->saveSettings();
                            printf("MPU is now %sreversed\n", power->reverseMPU ? "" : "not ");
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
                            print("Calibrate [a]ccel/gyro, [m]ag, [b]attery, [s]train, set [c]rank length, toggle [r]everse strain, toggle r[e]verse MPU, [p]rint calibration or e[x]it\n");
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
                                    print("Enter AP SSID (max 31 chars) and press [Enter]: ");
                                    getStr(wifi->settings.apSSID, 32);
                                    wifi->applySettings();
                                    wifi->saveSettings();
                                    menu[2] = '\0';
                                    break;
                                case 'p':
                                    print("Enter AP password (max 31 chars) and press [Enter]: ");
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
                                    print("AP setup: [e]nable, [s]SID, [p]assword or e[x]it\n");
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
                                    print("Enter STA SSID (max 31 chars) and press [Enter]: ");
                                    getStr(wifi->settings.staSSID, 32);
                                    wifi->applySettings();
                                    wifi->saveSettings();
                                    menu[2] = '\0';
                                    break;
                                case 'p':
                                    print("Enter STA password (max 31 chars) and press [Enter]: ");
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
                                    print("STA setup: [e]nable, [s]SID, [p]assword or e[x]it\n");
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
                            print("WiFi setup: [a]P, [s]TA, [p]rint config or e[x]it\n");
                            menu[1] = getChar();
                            menu[2] = '\0';
                    }
                    break;
                case 'r':
                    ESP.restart();
                    return;
                case NULL:
                    print("NULL received\n");
                    return;
                case 4:
                    print("Ctrl-D received\n");
                    // TODO disconnect client
                    return;
                default:
                    print("[c]alibrate, [w]iFi or [r]eboot\n");
                    return;
            }
        }
    }
};

#endif