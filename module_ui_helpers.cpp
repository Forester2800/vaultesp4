#include "module_ui_helpers.h"

#include "config.h"
#include "strings.h"

namespace {
constexpr uint8_t ST7789_CMD_SLPIN = 0x10;
constexpr uint8_t ST7789_CMD_SLPOUT = 0x11;
constexpr uint8_t ST7789_CMD_DISPOFF = 0x28;
constexpr uint8_t ST7789_CMD_DISPON = 0x29;
}

void applyThemeColor() {
    state.activeThemeColor = state.diagnosticTheme ? THEME_DIAGNOSTIC
                                                   : (state.amberTheme ? THEME_AMBER : THEME_GREEN);
}

void writeDisplayBacklight(bool on) {
    if (DISPLAY_BACKLIGHT_PIN < 0) {
        return;
    }
    digitalWrite(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_ACTIVE_LOW ? !on : on);
}

void configureDisplayPower() {
    applyThemeColor();
    tft.init();
    tft.setRotation(0);
    tft.setTextWrap(false);
    tft.fillScreen(BLACK);

    if (DISPLAY_BACKLIGHT_PIN >= 0) {
        pinMode(DISPLAY_BACKLIGHT_PIN, OUTPUT);
        writeDisplayBacklight(true);
    }
}

void displayWake() {
    applyThemeColor();
    if (state.displaySleeping) {
        tft.writecommand(ST7789_CMD_SLPOUT);
        vTaskDelay(DISPLAY_WAKE_DELAY_MS / portTICK_PERIOD_MS);
        tft.writecommand(ST7789_CMD_DISPON);
        state.displaySleeping = false;
    }
    writeDisplayBacklight(true);
}

void displaySleep() {
    tft.fillScreen(BLACK);
    tft.writecommand(ST7789_CMD_DISPOFF);
    vTaskDelay(DISPLAY_SLEEP_DELAY_MS / portTICK_PERIOD_MS);
    tft.writecommand(ST7789_CMD_SLPIN);
    writeDisplayBacklight(false);
    state.displaySleeping = true;
}

void loadCustomFont() {
    if (!state.displayOnline) {
        Serial.println("Дисплей недоступен, пропускаем loadCustomFont");
        return;
    }
    tft.setTextFont(1);
    tft.setTextSize(1);
}

void printAt(int16_t x, int16_t y, const char* text, uint16_t fg, uint16_t bg) {
    tft.setTextColor(fg, bg);
    tft.setCursor(x, y);
    tft.print(text);
}

void printAtCentered(int16_t y, const char* text, uint16_t fg, uint16_t bg) {
    tft.setTextColor(fg, bg);
    tft.drawCentreString(text, SCREEN_W / 2, y, 1);
}

void clearField(int16_t x, int16_t y, int16_t w, int16_t h) {
    tft.fillRect(x, y, w, h, BLACK);
}

void drawCrtBackdrop() {
    applyThemeColor();
    const uint16_t scanColor = state.amberTheme ? 0x4200 : 0x0200;
    for (int16_t y = 2; y < SCREEN_H; y += 4) {
        tft.drawFastHLine(0, y, SCREEN_W, scanColor);
    }
}

void clearScreenWithCrt() {
    tft.fillScreen(BLACK);
    drawCrtBackdrop();
}

const char* getVentReasonText(VentReason reason) {
    switch (reason) {
        case VENT_FORCED: return STR_VENT_REASON_FORCED;
        case VENT_MANUAL_OFF: return STR_VENT_REASON_MANUAL_OFF;
        case VENT_COOLING: return STR_VENT_REASON_COOLING;
        case VENT_BAD_AIR: return STR_VENT_REASON_CLEAN_ATMOSPHERE;
        case VENT_CRITICAL_AIR: return STR_VENT_REASON_EMERGENCY_PROTOCOL;
        case VENT_NIGHT: return STR_VENT_REASON_NIGHT_PROTOCOL;
        case VENT_OUTSIDE_HOT: return STR_VENT_REASON_OUTSIDE_HOT;
        case VENT_OUTSIDE_COLD: return STR_VENT_REASON_OUTSIDE_COLD;
        case VENT_OUTSIDE_WET: return STR_VENT_REASON_OUTSIDE_WET;
        case VENT_SENSOR_WAIT: return STR_VENT_REASON_SENSOR_WAIT;
        case VENT_CYCLE_GUARD: return STR_VENT_REASON_CYCLE_GUARD;
        case VENT_DAY_LOCK:
        default: return STR_VENT_REASON_DAY_LOCK;
    }
}

const char* gasStateText() {
    if (!state.mqBurnInDone) {
        return STR_GAS_STATE_WARMING_UP;
    }
    if (!state.mqReady) {
        return STR_GAS_STATE_CALIBRATING;
    }
    return STR_GAS_STATE_OK;
}
