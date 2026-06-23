#pragma once

// Relay GPIO output helpers.

#include "state.h"
#include "config.h"

namespace {
    inline void writeRelay(bool on) {
        digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? !on : on);
        vTaskDelay(100 / portTICK_PERIOD_MS); // Задержка для уменьшения индуктивных помех при переключении реле
    }
}

inline void initializeRelay() {
    pinMode(RELAY_PIN, OUTPUT);
    writeRelay(state.relayOutputOn); // Initial state is off
}

inline void setRelayOutput(bool on) {
  if (state.relayOutputOn != on) {
    state.relayStateChangedMs = millis();
  }
  state.relayOutputOn = on;
  writeRelay(on);
}
