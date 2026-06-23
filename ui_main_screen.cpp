#include "ui_main_screen.h"
#include "state.h"
#include "module_ui_helpers.h"
#include "config.h"

namespace {
    // --- Cached values to avoid unnecessary redraws ---
    float lastTempRoom = -99, lastTempOutside = -99;
    float lastHumidityRoom = -1, lastHumidityOutside = -1;
    int lastPressure = -1;
    int lastLight = -1;
    int lastCo2 = -1, lastAqi = -1;
    bool lastRelayState = false;
    bool lastManualMode = false;
    uint8_t lastScreen = -1;

    // --- Helper function to draw a single data box ---
    void drawDataBox(int16_t x, int16_t y, const char* label, const char* value, const char* unit, uint16_t color) {
        tft.setTextColor(color, TFT_BLACK);
        tft.setTextSize(1);
        tft.setCursor(x + 4, y + 4);
        tft.print(label);

        tft.setTextSize(2);
        tft.setCursor(x + 4, y + 18);
        tft.print(value);

        tft.setTextSize(1);
        int16_t unitWidth = strlen(unit) * 6; // Estimate width
        tft.setCursor(x + 112 - unitWidth, y + 26);
        tft.print(unit);
        
        tft.drawRect(x, y, 118, 40, color);
    }
}

void drawMainScreen(bool fullRedraw) {
    if (fullRedraw) {
        // Reset cached values on full redraw
        lastTempRoom = -99; lastTempOutside = -99;
        lastHumidityRoom = -1; lastHumidityOutside = -1;
        lastPressure = -1; lastLight = -1;
        lastCo2 = -1; lastAqi = -1;
        lastRelayState = false; lastManualMode = false;
        lastScreen = state.currentScreen;

        // Draw static frame and headers
        applyThemeColor();
        tft.drawRect(0, 0, SCREEN_W, SCREEN_H, state.activeThemeColor);
        tft.drawRect(1, 1, SCREEN_W - 2, SCREEN_H - 2, state.activeThemeColor);
        tft.setCursor(10, 10);
        tft.setTextColor(state.activeThemeColor, TFT_BLACK);
        tft.setTextSize(1);
        tft.print("АТМОСФЕРА - ЖИЛОЙ БЛОК");

        // Bottom status line
        tft.fillRect(2, SCREEN_H - 15, SCREEN_W - 4, 13, state.activeThemeColor);
        tft.setCursor(5, SCREEN_H - 12);
        tft.setTextColor(TFT_BLACK, state.activeThemeColor);
        tft.print("НАЖАТИЕ: ПРОГНОЗ | УДЕРЖАНИЕ: ПИТАНИЕ");
    }

    char val[16];

    // --- ROOM DATA ---
    if (fullRedraw || state.current.tempRoom != lastTempRoom) {
        lastTempRoom = state.current.tempRoom;
        snprintf(val, sizeof(val), "%.1f", lastTempRoom);
        drawDataBox(5, 25, "ТЕМПЕРАТУРА", val, "C", state.activeThemeColor);
    }
    if (fullRedraw || state.current.humRoom != lastHumidityRoom) {
        lastHumidityRoom = state.current.humRoom;
        snprintf(val, sizeof(val), "%.0f", lastHumidityRoom);
        drawDataBox(128, 25, "ВЛАЖНОСТЬ", val, "%", state.activeThemeColor);
    }

    // --- OUTSIDE DATA ---
    if (fullRedraw || state.current.tempOutside != lastTempOutside) {
        lastTempOutside = state.current.tempOutside;
        snprintf(val, sizeof(val), "%.1f", lastTempOutside);
        drawDataBox(5, 70, "ТЕМП. (ВНЕ)", val, "C", state.activeThemeColor);
    }
    if (fullRedraw || state.current.humOutside != lastHumidityOutside) {
        lastHumidityOutside = state.current.humOutside;
        snprintf(val, sizeof(val), "%.0f", lastHumidityOutside);
        drawDataBox(128, 70, "ВЛАЖН. (ВНЕ)", val, "%", state.activeThemeColor);
    }

    // --- AIR & LIGHT DATA ---
    const int currentPressure = state.current.pressureOutsideMm > 0 ? state.current.pressureOutsideMm : state.current.pressureRoomMm;
    if (fullRedraw || currentPressure != lastPressure) {
        lastPressure = currentPressure;
        snprintf(val, sizeof(val), "%d", lastPressure);
        drawDataBox(5, 115, "ДАВЛЕНИЕ", val, "мм", state.activeThemeColor);
    }
    if (fullRedraw || state.current.lightPercent != lastLight) {
        lastLight = state.current.lightPercent;
        snprintf(val, sizeof(val), "%d", lastLight);
        drawDataBox(128, 115, "ОСВЕЩЕНИЕ", val, "%", state.activeThemeColor);
    }

    // --- AIR QUALITY & RELAY ---
    if (fullRedraw || state.current.co2 != lastCo2 || state.current.gasAqi != lastAqi) {
        lastCo2 = state.current.co2;
        lastAqi = state.current.gasAqi;
        snprintf(val, sizeof(val), "%d/%d", lastCo2, lastAqi);
        drawDataBox(5, 160, "CO2/AQI", val, " ppm", state.activeThemeColor);
    }

    if (fullRedraw || state.relayOutputOn != lastRelayState || state.manualVentMode != lastManualMode) {
        lastRelayState = state.relayOutputOn;
        lastManualMode = state.manualVentMode;
        uint16_t color = lastRelayState ? TFT_RED : state.activeThemeColor;
        const char* status = lastRelayState ? "АКТИВНА" : "ОТКЛЮЧЕНА";
        const char* mode = lastManualMode ? "РУЧН." : "АВТО";
        snprintf(val, sizeof(val), "%s", status);
        drawDataBox(128, 160, "ПРИТОЧКА", val, mode, color);
    }
}
