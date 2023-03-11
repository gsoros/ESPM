#include "api.h"
#include "board.h"

void Api::setup(
    Api *instance,
    ::Preferences *p,
    const char *preferencesNS,
    const char *serviceUuid) {
    Atoll::Api::setup(instance, p, preferencesNS, &board.bleServer, serviceUuid);
    addCommand(Command("system", systemProcessor));
    addCommand(Command("wse", weightServiceProcessor));
    addCommand(Command("cs", calibrateStrainProcessor));
    addCommand(Command("tare", tareProcessor));
    addCommand(Command("cl", crankLengthProcessor));
    addCommand(Command("rs", reverseStrainProcessor));
    addCommand(Command("dp", doublePowerProcessor));
    addCommand(Command("sd", sleepDelayProcessor));
    addCommand(Command("hc", hallCharProcessor));
    addCommand(Command("ho", hallOffsetProcessor));
    addCommand(Command("ht", hallThresholdProcessor));
    addCommand(Command("htl", hallThresLowProcessor));
    addCommand(Command("st", strainThresholdProcessor));
    addCommand(Command("stl", strainThresLowProcessor));
    addCommand(Command("mdm", motionDetectionMethodProcessor));
    addCommand(Command("sleep", sleepProcessor));
    addCommand(Command("ntm", negativeTorqueMethodProcessor));
    addCommand(Command("at", autoTareProcessor));
    addCommand(Command("atd", autoTareDelayMsProcessor));
    addCommand(Command("atr", autoTareRangeGProcessor));
}

void Api::beforeBleServiceStart(BLEService *service) {
    // add api char for reading hall effect sensor measurements
    BleServer *bleServer = &board.bleServer;
    bleServer->hallChar = service->createCharacteristic(
        BLEUUID(HALL_CHAR_UUID),
        BLE_PROP::READ | BLE_PROP::INDICATE | BLE_PROP::NOTIFY);
    bleServer->hallChar->setCallbacks(&board.bleServer);
    uint8_t bytes[2];
    bytes[0] = 0 & 0xff;
    bytes[1] = (0 >> 8) & 0xff;
    bleServer->hallChar->setValue((uint8_t *)bytes, 2);  // set initial value
    BLEDescriptor *hallDesc = bleServer->hallChar->createDescriptor(
        BLEUUID(HALL_DESC_UUID),
        BLE_PROP::READ);
    char str[] = "Hall Effect Sensor reading";
    hallDesc->setValue((uint8_t *)str, strlen(str));
}

Api::Result *Api::systemProcessor(Message *msg) {
    if (msg->argStartsWith("hostname")) {
        char buf[sizeof(board.hostName)] = "";
        msg->argGetParam("hostname:", buf, sizeof(buf));
        if (0 < strlen(buf)) {
            // set hostname
            if (strlen(buf) < 2) return argInvalid();
            if (!isAlNumStr(buf)) return argInvalid();
            strncpy(board.hostName, buf, sizeof(board.hostName));
            board.saveSettings();
        }
        // get hostname
        strncpy(msg->reply, board.hostName, msgReplyLength);
        return success();
    } else if (msg->argIs("ota") || msg->argIs("OTA")) {
        log_i("entering ota mode");
        board.otaMode = true;
        board.wifi.autoStartOta = true;
        board.wifi.setEnabled(true, false);
        msg->replyAppend("ota");
        return success();
    }
    msg->replyAppend("|", true);
    msg->replyAppend("hostname[:str]|ota");
    return Atoll::Api::systemProcessor(msg);
}

Api::Result *Api::weightServiceProcessor(Message *msg) {
    // set value
    if (0 < strlen(msg->arg)) {
        uint8_t newValue = (uint8_t)atoi(msg->arg);
        if (0 <= newValue && newValue < WM_MAX)
            board.bleServer.setWmCharMode(newValue);
        if (WM_OFF == newValue) board.bleServer.setWmValue(0.0F);
    }
    // get value
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", board.bleServer.wmCharMode);
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::calibrateStrainProcessor(Message *msg) {
    Api::Result *result = argInvalid();
    float knownMass;
    knownMass = (float)atof(msg->arg);
    if (1 < knownMass && knownMass < 1000) {
        if (0 == board.strain.calibrateTo(knownMass)) {
            board.strain.saveSettings();
            result = success();
        } else
            result = internalError();
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%f", knownMass);
    msg->replyAppend(buf);
    return result;
}

Api::Result *Api::tareProcessor(Message *msg) {
    if (0 < strlen(msg->arg)) {
        board.strain.tare();
        return success();
    }
    return argInvalid();
}

Api::Result *Api::crankLengthProcessor(Message *msg) {
    Api::Result *result = success();
    if (1 < strlen(msg->arg)) {
        result = error();
        float crankLength = (float)atof(msg->arg);
        if (10 < crankLength && crankLength < 2000) {
            board.power.crankLength = crankLength;
            board.power.saveSettings();
            result = success();
        }
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", board.power.crankLength);
    msg->replyAppend(buf);
    return result;
}

Api::Result *Api::reverseStrainProcessor(Message *msg) {
    bool newValue = false;  // disable by default
    if (0 < strlen(msg->arg)) {
        if (0 == strcmp("true", msg->arg) || 0 == strcmp("1", msg->arg)) {
            newValue = true;
        }
        board.power.reverseStrain = newValue;
        board.power.saveSettings();
    }
    char buf[4];
    snprintf(buf, sizeof(buf), "%d",
             (int)board.power.reverseStrain);
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::doublePowerProcessor(Message *msg) {
    bool newValue = false;  // disable by default
    if (0 < strlen(msg->arg)) {
        if (0 == strcmp("true", msg->arg) || 0 == strcmp("1", msg->arg)) {
            newValue = true;
        }
        board.power.reportDouble = newValue;
        board.power.saveSettings();
    }
    char buf[4];
    snprintf(buf, sizeof(buf), "%d",
             (int)board.power.reportDouble);
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::sleepDelayProcessor(Message *msg) {
    Api::Result *result = success();
    if (1 < strlen(msg->arg)) {
        result = error();
        ulong sleepDelay = (ulong)atoi(msg->arg);
        if (SLEEP_DELAY_MIN < sleepDelay) {
            board.sleepDelay = sleepDelay;
            board.saveSettings();
            result = success();
        }
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", board.sleepDelay);
    msg->replyAppend(buf);
    return result;
}

Api::Result *Api::hallCharProcessor(Message *msg) {
    bool newValue = false;  // disable by default
    if (0 < strlen(msg->arg)) {
        if (0 == strcmp("true", msg->arg) || 0 == strcmp("1", msg->arg)) {
            newValue = true;
        }
        board.bleServer.hallCharUpdateEnabled = newValue;
    }
    char buf[4];
    snprintf(buf, sizeof(buf), "%d",
             (int)board.bleServer.hallCharUpdateEnabled);
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::hallOffsetProcessor(Message *msg) {
    if (0 < strlen(msg->arg)) {
        board.motion.hallOffset = atoi(msg->arg);
        board.motion.saveSettings();
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", board.motion.hallOffset);
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::hallThresholdProcessor(Message *msg) {
    if (0 < strlen(msg->arg)) {
        board.motion.setHallThreshold(atoi(msg->arg));
        board.motion.saveSettings();
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", board.motion.hallThreshold);
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::hallThresLowProcessor(Message *msg) {
    if (0 < strlen(msg->arg)) {
        board.motion.setHallThresLow(atoi(msg->arg));
        board.motion.saveSettings();
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", board.motion.hallThresLow);
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::strainThresholdProcessor(Message *msg) {
    if (0 < strlen(msg->arg)) {
        board.strain.setMdmStrainThreshold(atoi(msg->arg));
        board.strain.saveSettings();
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", board.strain.mdmStrainThreshold);
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::strainThresLowProcessor(Message *msg) {
    if (0 < strlen(msg->arg)) {
        board.strain.setMdmStrainThresLow(atoi(msg->arg));
        board.strain.saveSettings();
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", board.strain.mdmStrainThresLow);
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::motionDetectionMethodProcessor(Message *msg) {
    Api::Result *result = error();
    if (0 < strlen(msg->arg)) {
        int tmpI = atoi(msg->arg);
        if (0 <= tmpI && tmpI < MDM_MAX) {
            board.setMotionDetectionMethod(tmpI);
            board.saveSettings();
            result = success();
        } else
            result = argInvalid();
    } else
        result = success();
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", board.motionDetectionMethod);
    msg->replyAppend(buf);
    return result;
}

Api::Result *Api::sleepProcessor(Message *msg) {
    Api::Result *result = argInvalid();
    if (0 == strcmp("true", msg->arg) || 0 == strcmp("1", msg->arg)) {
        result = board.deepSleep() == 0 ? success() : error();
    }
    msg->replyAppend("0");
    return result;
}

Api::Result *Api::negativeTorqueMethodProcessor(Message *msg) {
    Api::Result *result = error();
    if (0 < strlen(msg->arg)) {
        int tmpI = atoi(msg->arg);
        if (0 <= tmpI && tmpI < NTM_MAX) {
            board.strain.negativeTorqueMethod = tmpI;
            board.strain.saveSettings();
            result = success();
        } else
            result = argInvalid();
    } else
        result = success();
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", board.strain.negativeTorqueMethod);
    msg->replyAppend(buf);
    return result;
}

Api::Result *Api::autoTareProcessor(Message *msg) {
    if (0 < strlen(msg->arg)) {
        int i = atoi(msg->arg);
        if (0 == strcmp("true", msg->arg) || 1 == i)
            board.strain.setAutoTare(true);
        else if (0 == strcmp("false", msg->arg) || 0 == i)
            board.strain.setAutoTare(false);
        board.strain.saveSettings();
    }
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", (int)board.strain.getAutoTare());
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::autoTareDelayMsProcessor(Message *msg) {
    if (0 < strlen(msg->arg)) {
        int tmpI = atoi(msg->arg);
        if (10 < tmpI && tmpI < 10000) {
            board.strain.setAutoTareDelayMs(tmpI);
            board.strain.saveSettings();
        }
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu", board.strain.getAutoTareDelayMs());
    msg->replyAppend(buf);
    return success();
}

Api::Result *Api::autoTareRangeGProcessor(Message *msg) {
    if (0 < strlen(msg->arg)) {
        int tmpI = atoi(msg->arg);
        if (10 < tmpI && tmpI < 10000) {
            board.strain.setAutoTareRangeG(tmpI);
            board.strain.saveSettings();
        }
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", (int)board.strain.getAutoTareRangeG());
    msg->replyAppend(buf);
    return success();
}
