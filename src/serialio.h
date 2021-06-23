#ifndef SERIALIO_H
#define SERIALIO_H

#include "battery.h"
#include "mpu.h"
#include "strain.h"
#include "wificonnection.h"
#include <Arduino.h>

class SerialIO
{
public:
    Battery *battery;
    MPU *mpu;
    Strain *strain;
    WifiConnection *wifi;
    bool statusEnabled = false;

    void setup(Battery *b, MPU *m, Strain *s, WifiConnection *w)
    {
        battery = b;
        mpu = m;
        strain = s;
        wifi = w;
        Serial.begin(115200);
        while (!Serial)
            delay(1);
        statusEnabled = true;
    }

    void loop(const ulong t)
    {
        if (Serial.available() > 0)
        {
            handleInput(getChar());
        }
        printStatus(t);
    }

    char getChar()
    {
        while (!Serial.available())
        {
            delay(10);
        }
        return Serial.read();
    }

    int getStr(char *str, int maxLength, bool echo = true)
    {
        int received = -1;
        char buffer[maxLength];
        char c;
        while (1)
        {
            c = getChar();
            received++;
            if ('\n' == c)
            {
                break;
            }
            if ('\r' == c)
            {
                received--;
                continue;
            }
            if (received >= maxLength)
            {
                break;
            }
            if (echo)
            {
                Serial.print(c);
            }
            buffer[received] = c;
        }
        buffer[received] = '\0';
        strncpy(str, buffer, maxLength);
        return received;
    }

    void printStatus(const ulong t)
    {
        static ulong lastOutput = 0;
        if (!statusEnabled)
            return;
        if (lastOutput < t - 2000)
        {
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
        e[x]it
    */
    void handleInput(const char input)
    {
        char menu[8];
        strncpy(menu, &input, 1);
        menu[1] = '\0';
        while (1)
        {
            switch (menu[0])
            {
            case 'c':
                switch (menu[1])
                {
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
                case 'x':
                    strncpy(menu, " ", 2);
                    break;
                default:
                    printf("Calibrate [a]ccel/gyro, [m]ag or e[x]it\n");
                    menu[1] = getChar();
                    menu[2] = '\0';
                }
                break;
            case 'w':
                switch (menu[1])
                {
                case 'a':
                    switch (menu[2])
                    {
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
                    switch (menu[2])
                    {
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
            case 'x':
                printf("Exit menu\n");
                return;
            default:
                printf("[c]alibrate, [w]iFi, [r]eboot or e[x]it\n");
                menu[0] = getChar();
                menu[1] = '\0';
            }
        }
    }

    /*
    void handleInput(const char input = '\0')
    {
        const uint8_t menumax = 8;
        static char submenu[menumax] = "       ";
        switch ('\0' == input ? getChar() : input)
        {
        case 'w':
            //strncpy(submenu, "w", menumax);
            switch (submenu[0])
            {
            case 'x':
                submenu[0] = ' ';
                break;
            case 'a':
                submenu[0] = 'a';
                Serial.println("Access point: toggle [e]nable, set [s]sid, set [p]assword, e[x]it");
                handleInput('w');
                break;
            default:
                Serial.println("configure [a]p, configure [s]ta, [p]rint config, e[x]it");
                handleInput('w');
            }
            break;
        case 's':
            statusEnabled = !statusEnabled;
            if (!statusEnabled)
                Serial.println("Status output paused, press [s] to resume");
            break;
        case 'c':
            mpu->calibrate();
            //handleInput('h');
            break;
        case 'h':
            Serial.println("[c]alibrate, [w]ifi, [s]tatus, [r]eboot");
            break;
        }
    }
    */
};

#endif