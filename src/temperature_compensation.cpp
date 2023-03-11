#ifdef FEATURE_TEMPERATURE_COMPENSATION

#include "temperature_compensation.h"
#include "board.h"

TemperatureCompensation::TemperatureCompensation(
    uint16_t size,
    int16_t keyOffset,
    float keyResolution,
    float valueResolution) {
    setSize(size);
    setKeyOffset(keyOffset);
    setKeyResolution(keyResolution);
    setValueResolution(valueResolution);
}

TemperatureCompensation::~TemperatureCompensation() {
    free(values);
}

void TemperatureCompensation::setup(::Preferences *p) {
    uint32_t heap = esp_get_free_heap_size();
    int stack = (int)uxTaskGetStackHighWaterMark(NULL);
    preferencesSetup(p, "TC");
    loadSettings();
    printSettings();
    addApiCommand();
    log_d("heap used: %d, stack used: %d",
          heap - esp_get_free_heap_size(),
          stack - (int)uxTaskGetStackHighWaterMark(NULL));
}

bool TemperatureCompensation::setSize(uint16_t size) {
    if (!size) {
        if (nullptr != values) {
            free(values);
            values = nullptr;
        }
        this->size = 0;
        return true;
    }
    if (nullptr == values)
        values = (int8_t *)malloc(size * sizeof(int8_t));
    else if (size == getSize()) {
        return true;
    } else
        values = (int8_t *)realloc(values, size * sizeof(int8_t));
    if (nullptr == values) {
        log_e("memory allocation failed");
        this->size = 0;
        return false;
    }
    this->size = size;
    for (uint16_t i = 0; i < size; i++) {
        // unsetValue(i);
        setValue(i, (int8_t)random(valueUnset, INT8_MAX + 1));
    }
    return true;
}

uint16_t TemperatureCompensation::getSize() {
    return size;
}

int16_t TemperatureCompensation::getKeyOffset() {
    return keyOffset;
}

bool TemperatureCompensation::setKeyOffset(int16_t offset) {
    keyOffset = offset;
    return true;
}

float TemperatureCompensation::getKeyResolution() {
    return keyResolution;
}

bool TemperatureCompensation::setKeyResolution(float resolution) {
    keyResolution = resolution;
    return true;
}

float TemperatureCompensation::getValueResolution() {
    return valueResolution;
}

bool TemperatureCompensation::setValueResolution(float resolution) {
    valueResolution = resolution;
    return true;
}

bool TemperatureCompensation::validIndex(uint16_t index) {
    return index < getSize();
}

int8_t TemperatureCompensation::getValue(uint16_t index) {
    if (!validIndex(index)) {
        log_e("index %d out of range", index);
        return valueUnset;
    }
    return values[index];
}

bool TemperatureCompensation::setValue(uint16_t index, int8_t value) {
    if (!validIndex(index)) {
        log_e("index %d out of range", index);
        return false;
    }
    values[index] = value;
    return true;
}

bool TemperatureCompensation::unsetValue(uint16_t index) {
    return setValue(index, valueUnset);
}

void TemperatureCompensation::loadSettings() {
    if (!preferencesStartLoad()) return;
    if (preferences->isKey("enabled"))
        setSize(preferences->getBool("enabled", enabled));
    if (preferences->isKey("size"))
        setSize(preferences->getUInt("size", getSize()));
    if (preferences->isKey("keyO"))
        setKeyOffset(preferences->getUInt("keyO", getKeyOffset()));
    if (preferences->isKey("keyR"))
        setKeyResolution(preferences->getFloat("keyR", getKeyResolution()));
    if (preferences->isKey("valueR"))
        setValueResolution(preferences->getFloat("valueR", getValueResolution()));
    if (preferences->isKey("values"))
        preferences->getBytes("values", values, getSize() * sizeof(int8_t));
    preferencesEnd();
}

void TemperatureCompensation::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putBool("enabled", enabled);
    preferences->putUInt("size", getSize());
    preferences->putUInt("keyO", getKeyOffset());
    preferences->putFloat("keyR", getKeyResolution());
    preferences->putFloat("valueR", getValueResolution());
    preferences->putBytes("values", values, getSize() * sizeof(int8_t));
    preferencesEnd();
}

void TemperatureCompensation::printSettings() {
    int8_t cur, min = valueMax, max = valueMin;
    uint16_t numSet = 0;
    for (uint16_t i = 0; i < getSize(); i++) {
        cur = getValue(i);
        if (valueUnset == cur) continue;
        numSet++;
        if (cur < min) min = cur;
        if (max < cur) max = cur;
    }
    char range[32];
    snprintf(range, sizeof(range), " (%d - %d)", min, max);
    char buf[128];
    snprintf(buf, sizeof(buf), "%d values%s", numSet, numSet ? range : "");
    log_i("enabled: %d, table size: %d, key offset: %d, key res: %.2f˚C, value res: %.2fkg, %s",
          enabled,
          getSize(),
          getKeyOffset(),
          getKeyResolution(),
          getValueResolution(),
          buf);
}

void TemperatureCompensation::addApiCommand() {
    // board.api.addCommand(Api::Command("tc", tcProcessor));
    board.api.addCommand(Api::Command("tc", [this](Api::Message *m) { return tcProcessor(m); }));
}

Api::Result *TemperatureCompensation::tcProcessor(Api::Message *msg) {
    // get/set enabled: tc=[0|1] -> =0|1
    if (msg->argIs("") || msg->argIs("0") || msg->argIs("1")) {
        if (msg->argIs("0") || msg->argIs("1")) {
            enabled = atoi(msg->arg);
            saveSettings();
        }
        snprintf(msg->reply, sizeof(msg->reply), "%d=%d", Api::command("tc")->code, enabled);
        return Api::success();
    }

    // get/set params: params[;size:512;keyOffset:-15;keyRes:0.1;valueRes:0.2;] -> =size:512;keyOffset:-15;keyRes:0.1;valueRes:0.2;
    if (msg->argStartsWith("params")) {
        if (msg->argStartsWith("params;")) {
            uint8_t changed = 0;
            char buf[8] = "";
            if (msg->argHasParam("size:")) {
                msg->argGetParam("size:", buf, sizeof(buf));
                int i = atoi(buf);
                if (i < 16 || 10000 < i) {
                    msg->replyAppend("size out of range (16-10000)");
                    return Api::argInvalid();
                }
                if (i != getSize()) {
                    setSize(i);
                    changed++;
                }
            }
            if (msg->argHasParam("keyOffset:")) {
                msg->argGetParam("keyOffset:", buf, sizeof(buf));
                int i = atoi(buf);
                if (i < INT16_MIN || INT16_MAX < i) {
                    msg->replyAppend("keyOffset out of range (int16)");
                    return Api::argInvalid();
                }
                if (i != getKeyOffset()) {
                    setKeyOffset(i);
                    changed++;
                }
            }
            if (msg->argHasParam("keyRes:")) {
                msg->argGetParam("keyRes:", buf, sizeof(buf));
                double f = atof(buf);
                if (f < 0.01 || 1.0 < f) {
                    msg->replyAppend("keyRes out of range (0.01-1.0)");
                    return Api::argInvalid();
                }
                if (f != getKeyResolution()) {
                    setKeyResolution(f);
                    changed++;
                }
            }
            if (msg->argHasParam("valueRes:")) {
                msg->argGetParam("valueRes:", buf, sizeof(buf));
                double f = atof(buf);
                if (f < 0.01 || 1.0 < f) {
                    msg->replyAppend("valueRes out of range (0.01-1.0)");
                    return Api::argInvalid();
                }
                if (f != getValueResolution()) {
                    setValueResolution(f);
                    changed++;
                }
            }
            if (changed) saveSettings();
        }
        snprintf(msg->reply, sizeof(msg->reply), "%d=size:%d;keyOffset:%d;keyRes:%.2f;valueRes:%.2f;",
                 Api::command("tc")->code,
                 getSize(),
                 getKeyOffset(),
                 getKeyResolution(),
                 getValueResolution());
        return Api::success();
    }

    // get/set values: tc=values[;start:index[;set:val1,val2,val3,val4,...]] -> =values;start:index;val1,val2,val3,...
    if (msg->argStartsWith("values")) {
        if (msg->argStartsWith("values;")) {
            char buf[512] = "";
            log_i("TODO values");
            return Api::internalError();
        }
    }

    msg->replyAppend("0|1|params[;size:512;keyOffset:-15;keyRes:0.1;valueRes:0.2;]|values[;start:index[;set:val1,val2,...]]");
    return Api::argInvalid();
}

#endif  // FEATURE_TEMPERATURE_COMPENSATION