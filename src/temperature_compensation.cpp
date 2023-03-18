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
        unsetValue(i);
        // setValue(i, (int8_t)random(valueMin, valueMax +1));
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
        return 0;
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
    return setValue(index, 0);
}

void TemperatureCompensation::loadSettings() {
    if (!preferencesStartLoad()) return;
    if (preferences->isKey("enabled"))
        enabled = preferences->getBool("enabled", enabled);
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
    for (uint16_t i = 0; i < getSize(); i++) {
        cur = getValue(i);
        if (cur < min) min = cur;
        if (max < cur) max = cur;
    }
    log_i("enabled: %d, table size: %d, key offset: %d, key res: %.2f˚C, value res: %.2fkg, range %d...%d",
          enabled,
          getSize(),
          getKeyOffset(),
          getKeyResolution(),
          getValueResolution(),
          min,
          max);
}

void TemperatureCompensation::addApiCommand() {
    // board.api.addCommand(Api::Command("tc", tcProcessor));
    board.api.addCommand(Api::Command("tc", [this](Api::Message *m) { return tcProcessor(m); }));
}

Api::Result *TemperatureCompensation::tcProcessor(Api::Message *msg) {
    // get/set enabled: tc=[enabled][:0|1] -> enabled:0|1
    if (msg->argIs("") || msg->argIs("0") || msg->argIs("1") || msg->argStartsWith("enabled")) {
        if (strlen(msg->arg)) {
            int8_t tmpInt = -1;
            if (msg->argIs("1"))
                tmpInt = 1;
            else if (msg->argIs("0"))
                tmpInt = 0;
            else if (msg->argHasParam("enabled:")) {
                char buf[2] = "";
                msg->argGetParam("enabled:", buf, sizeof(buf));
                tmpInt = atoi(buf);
            }
            if (0 <= tmpInt && tmpInt <= 1) {
                enabled = (bool)tmpInt;
                saveSettings();
            }
        }
        snprintf(msg->reply, sizeof(msg->reply), "enabled:%d", enabled);
        return Api::success();
    }

    // get/set table params: table[;size:512;keyOffset:-15;keyRes:0.1;valueRes:0.2;] -> table;size:512;keyOffset:-15;keyRes:0.1;valueRes:0.2;
    if (msg->argStartsWith("table")) {
        if (msg->argStartsWith("table;")) {
            log_d("arg: %s", msg->arg);
            uint8_t changed = 0;
            char buf[8] = "";
            if (msg->argGetParam("size:", buf, sizeof(buf))) {
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
            if (msg->argGetParam("keyOffset:", buf, sizeof(buf))) {
                int i = atoi(buf);
                log_d("got keyOffset: %d", i);
                if (i < INT16_MIN || INT16_MAX < i) {
                    msg->replyAppend("keyOffset out of range (int16)");
                    return Api::argInvalid();
                }
                if (i != getKeyOffset()) {
                    setKeyOffset(i);
                    changed++;
                }
            }
            if (msg->argGetParam("keyRes:", buf, sizeof(buf))) {
                double f = atof(buf);
                log_d("got keyRes: %.4f", f);
                if (f < 0.001 || 1.0 < f) {
                    msg->replyAppend("keyRes out of range (0.001...1.0)");
                    return Api::argInvalid();
                }
                if ((float)f != getKeyResolution()) {
                    setKeyResolution(f);
                    changed++;
                }
            }
            if (msg->argGetParam("valueRes:", buf, sizeof(buf))) {
                double f = atof(buf);
                log_d("got valueRes: %.4f", f);
                if (f < 0.001 || 1.0 < f) {
                    msg->replyAppend("valueRes out of range (0.001...1.0)");
                    return Api::argInvalid();
                }
                if ((float)f != getValueResolution()) {
                    setValueResolution(f);
                    changed++;
                }
            }
            if (changed) saveSettings();
        }
        snprintf(msg->reply, sizeof(msg->reply), "table;size:%d;keyOffset:%d;keyRes:%.4f;valueRes:%.4f;",
                 getSize(),
                 getKeyOffset(),
                 getKeyResolution(),
                 getValueResolution());
        return Api::success();
    }

    // get/set values: tc=valuesFrom:index[;set:val1,val2,val3,...]] -> valuesFrom:index;val1,val2,val3,...
    if (msg->argStartsWith("valuesFrom:")) {
        char buf[6] = "";  // "65535" "-123,"
        msg->argGetParam("valuesFrom:", buf, sizeof(buf));
        int index = atoi(buf);
        if (index < 0 || getSize() - 1 < index) goto indexOutOfRange;
        if (msg->argHasParam("set:")) {
            bool changed = false;
            uint16_t writeIndex = index;
            char buf[5] = "";  // "-123"
            char *start = strstr(msg->arg, "set:");
            char *end;
            if (start) {
                start += strlen("set:");
                end = strstr(start, ",");
                if (!end) end = start + strlen(start);
                int value;
                int8_t oldValue;
                bool done = false;
                while (true) {
                    if (getSize() - 1 < writeIndex) goto indexOutOfRange;
                    snprintf(buf, min(sizeof(buf), (uint)(end - start) + 1), "%s", start);
                    if (0 == strcmp(buf, "") || 0 == strcmp(buf, " "))
                        value = 0;
                    else
                        value = atoi(buf);
                    if (value < valueMin || valueMax < value) {
                        char buf[40];
                        snprintf(buf, sizeof(buf), "value %d out of range (%d - %d)",
                                 value, valueMin, valueMax);
                        msg->replyAppend(buf);
                        return Api::argInvalid();
                    }
                    log_d("index: %d, value: %d", writeIndex, value);
                    oldValue = getValue(writeIndex);
                    setValue(writeIndex, value);
                    // if (false) changed = true;
                    if (oldValue != value) changed = true;
                    if (done) break;
                    start = end + 1;
                    end = strstr(start, ",");
                    if (!end) {
                        end = start + strlen(start) + 1;
                        done = true;
                    }
                    writeIndex++;
                };
            }
            if (changed) saveSettings();
        }
        {
            snprintf(msg->reply, sizeof(msg->reply), "valuesFrom:%d;", index);
            int8_t value;
            int avail = sizeof(msg->reply) - strlen(msg->reply) - 10;
            uint16_t size = getSize();
            for (int16_t i = index; i < size; i++) {
                value = getValue(i);
                if (0 == value && i + 1 < size)
                    snprintf(buf, sizeof(buf), ",", value);
                else if (i + 1 < size)
                    snprintf(buf, sizeof(buf), "%d,", value);
                else
                    snprintf(buf, sizeof(buf), "%d", value);  // last value
                avail -= strlen(buf);
                if (avail <= 0) break;
                msg->replyAppend(buf);
            }
        }
        return Api::success();

    indexOutOfRange : {
        char buf[32];
        snprintf(buf, sizeof(buf), "index out of range (0 - %d)", getSize() - 1);
        msg->replyAppend(buf);
        return Api::argInvalid();
    }
    }
    msg->replyAppend("[enabled][:0|1]|table[;size:uint16;keyOffset:int8;keyRes:float;valueRes:float;]|valuesFrom:uint16[;set:[int8],[int8],...]");
    return Api::argInvalid();

    /*
    tc=valuesFrom:1;set:-111,-87,-1,0,0,12,22,33,250     // value out of range
    tc=valuesFrom:-1;set:250                             // index out of range
    tc=valuesFrom:65535;set:-150                         // index out of range
    tc=valuesFrom:0;set:,, ,,100,,                       // ok
    tc=valuesFrom:0;set: ,, ,,,,,                        // ok
    tc=valuesFrom:0;set:-90,-80,-70,-60,-50,-40,-30,-20,-10,,10,20,30,40,50,60,70,80,90,100
    tc=valuesFrom:20;set:-90,,23,24,27,32,33,31,30,26,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
    tc=valuesFrom:40;set:-90,-80,-70,-60,-50,-40,-30,-20,-10,0,10,20,30,40,50,60,70,80,90,100
    tc=valuesFrom:60;set:-90,-80,-70,-60,-50,-40,-30,-20,-10,0,10,20,30,40,50,60,70,80,90,100
    tc=valuesFrom:80;set:-90,-80,-70,-60,-50,-40,-30,-20,-10,0,10,20,30,40,50,60,70,80,90,100
    tc=valuesFrom:100;set:-90,-80,-70,-60,-50,-40,-30,-20,-10,0,10,20,30,40,50,60,70,80,90,100
    tc=table;size:100;keyOffset:-15;keyRes:0.5;valueRes:0.1;
    tc=table;size:69;keyOffset:15;keyRes:0.5;valueRes:0.1;
    tc=table;size:70;keyOffset:17;keyRes:0.4429;valueRes:0.1175;
    tc=valuesFrom:0;set:-5,0,1,2,3,4,5,6,7,8,9,10,12,12,15,15,17,18,19,20,21,22,23,24,26,27,28,29,31,33,34,35,37,39,40,42,44,47,49,50,52,54,57,59,62,64,65,67,69,71,73,77,79,81,83,85,86,88,89,91,95,97,99,100,102,104,115,118,122,126
    tc=table;size:69;keyOffset:17;keyRes:0.4493;valueRes:0.1174;
    tc=valuesFrom:0;-2,-1,-1,2,3,4,5,6,7,8,9,10,11,12,15,15,17,18,19,20,21,22,23,25,26,27,28,30,31,33,34,36,38,39,41,43,45,47,49,51,53,56,58,60,63,65,66,69,70,72,77,79,81,83,84,86,87,89,91,94,96,98,100,102,104,114,118,122,126
    */
}

#endif  // FEATURE_TEMPERATURE_COMPENSATION