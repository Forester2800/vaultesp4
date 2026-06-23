#include "state.h"

SemaphoreHandle_t sharedStateMux = NULL;

void createSharedStateMux() {
    if (sharedStateMux == NULL) {
        sharedStateMux = xSemaphoreCreateMutex();
    }
}

void copySharedReadings(Readings& readings, VentDecision& decision) {
    if (LOCK_STATE()) {
        readings = state.current;
        decision = state.ventDecision;
        UNLOCK_STATE();
        return;
    }
    readings = state.current;
    decision = state.ventDecision;
}

void publishSensorReadings(const Readings& nextReadings) {
    if (LOCK_STATE()) {
        state.previous = state.current;
        state.current = nextReadings;
        state.current.co2 = state.current.gasAqi;
        UNLOCK_STATE();
        return;
    }
    state.previous = state.current;
    state.current = nextReadings;
    state.current.co2 = state.current.gasAqi;
}

void publishVentDecision(const VentDecision& decision) {
    if (LOCK_STATE()) {
        state.ventDecision = decision;
        state.current.relayOn = decision.relayOn;
        UNLOCK_STATE();
        return;
    }
    state.ventDecision = decision;
    state.current.relayOn = decision.relayOn;
}
