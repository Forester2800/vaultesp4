#include "module_controls.h"

#include "config.h"
#include "module_ui_helpers.h"

namespace {
bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;
unsigned long lastDebounceMs = 0;
unsigned long buttonPressedAtMs = 0;
unsigned long lastButtonClickMs = 0;
uint8_t pendingButtonClicks = 0;
bool longPressHandled = false;

void applyPendingClicks() {
    const uint8_t clicks = pendingButtonClicks;
    pendingButtonClicks = 0;

    if (clicks == 0) {
        return;
    }

    if (clicks == 1) {
        state.currentScreen = (state.currentScreen + 1) % 3;
        return;
    }

    if (clicks == 2) {
        state.manualVentMode = !state.manualVentMode;
        if (!state.manualVentMode) {
            state.manualRelayOn = false;
            state.manualVentUntilMs = 0;
        }
        return;
    }

    state.amberTheme = !state.amberTheme;
    applyThemeColor();
}
}

void setManualRelay(bool on, uint32_t timeoutMs) {
    state.manualVentMode = true;
    state.manualRelayOn = on;
    state.manualVentUntilMs = (timeoutMs > 0) ? (millis() + timeoutMs) : 0;
}

void maintainManualVentTimeout() {
    const bool shouldReset =
        state.manualVentMode && state.manualVentUntilMs != 0 && millis() > state.manualVentUntilMs;

    if (shouldReset) {
        // При истечении таймера ручной режим остается активным, реле выключается
        state.manualRelayOn = false;
        state.manualVentUntilMs = 0;
        // state.manualVentMode остается true! Система не прыгает в авторежим

        // Отправляем JSON-пакет обновления статуса на сервер
        Serial.print("{\"type\":\"status_update\",\"manual_mode\":true,\"relay_on\":false}\n");
    }
}

void handleScreenButton() {
    const bool reading = digitalRead(BUTTON_PIN);
    const unsigned long now = millis();

    if (reading != lastButtonReading) {
        lastDebounceMs = now;
        lastButtonReading = reading;
    }

    if ((now - lastDebounceMs) < DEBOUNCE_MS) {
        maintainManualVentTimeout();
        return;
    }

    if (reading != stableButtonState) {
        stableButtonState = reading;

        if (stableButtonState == LOW) {
            buttonPressedAtMs = now;
            longPressHandled = false;
        } else {
            if (!longPressHandled && !state.displaySleeping) {
                pendingButtonClicks++;
                lastButtonClickMs = now;
            }
            buttonPressedAtMs = 0;
            longPressHandled = false;
        }
    }

    if (stableButtonState == LOW && !longPressHandled && (now - buttonPressedAtMs >= POWER_HOLD_MS)) {
        longPressHandled = true;
        if (state.displaySleeping) {
            displayWake();
        } else {
            displaySleep();
        }
    }

    if (!state.displaySleeping && pendingButtonClicks > 0 && (now - lastButtonClickMs > DOUBLE_CLICK_MS)) {
        applyPendingClicks();
    }

    maintainManualVentTimeout();
}

void initializeControls() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}
