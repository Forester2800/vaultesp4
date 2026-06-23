#include "module_persistence.h"

#include "config.h"
#include "strings.h"

extern Preferences prefs;

const char* faultCodeText(FaultCode code) {
    switch (code) {
        case FAULT_BME_ROOM:
            return "BME_ROOM_FAIL";
        case FAULT_BME_OUTSIDE:
            return "BME_OUTSIDE_FAIL";
        case FAULT_ADC_MQ:
            return "ADC_MQ_FAIL";
        case FAULT_NONE:
        default:
            return "NONE";
    }
}

void saveLastFault(FaultCode code) {
    state.lastFaultCode = code;
    prefs.begin(STR_FAULT_PREFS_NAMESPACE, false);
    prefs.putUChar("last", static_cast<uint8_t>(code));
    prefs.end();
}

void loadLastFault() {
    prefs.begin(STR_FAULT_PREFS_NAMESPACE, true);
    state.lastFaultCode = static_cast<FaultCode>(prefs.getUChar("last", static_cast<uint8_t>(FAULT_NONE)));
    prefs.end();
}

void saveMqBurnInState() {
    prefs.begin(STR_MQ_PREFS_NAMESPACE, false);
    prefs.putBool("burnInDone", state.mqBurnInDone);
    prefs.end();
}

void loadMqBurnInState() {
    prefs.begin(STR_MQ_PREFS_NAMESPACE, true);
    state.mqBurnInDone = prefs.getBool("burnInDone", false);
    prefs.end();
    state.mqReady = state.mqBurnInDone;
}

void saveMqCalibrationBaseline() {
    prefs.begin(STR_MQ_PREFS_NAMESPACE, false);
    prefs.putULong("baseline", static_cast<uint32_t>(state.mqBaselineAqi));
    prefs.end();
}

void loadMqCalibrationBaseline() {
    prefs.begin(STR_MQ_PREFS_NAMESPACE, true);
    state.mqBaselineAqi = static_cast<int>(prefs.getULong("baseline", 0));
    prefs.end();
}

void updateMqBurnInState() {
    if (state.mqBurnInDone) {
        state.mqReady = true;
        return;
    }

    state.mqReady = false;
}
