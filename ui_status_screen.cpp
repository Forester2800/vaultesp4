#include "ui_status_screen.h"
#include "state.h"
#include "module_ui_helpers.h"
#include "config.h"
#include "strings.h"

namespace {
    // --- Cached values to avoid unnecessary redraws ---
    ControllerState lastControllerState = CTRL_BOOT;
    VentReason lastVentReason = VENT_SENSOR_WAIT;
    int lastTimeSyncState = -1;
    bool lastBmeRoom = false, lastBmeOut = false, lastMq = false, lastLight = false, lastRelay = false;
    int lastHeap = -1;
    uint8_t lastScreen = -1;

    const char* getOnOff(bool status) {
        return status ? "OK" : "FAIL";
    }

    const char* getControllerStateName(ControllerState value) {
        switch (value) {
            case CTRL_BOOT: return "BOOT";
            case CTRL_WARMUP: return "WARMUP";
            case CTRL_NORMAL: return "NORMAL";
            case CTRL_OFFLINE_LOGGING: return "OFFLINE";
            case CTRL_FAILSAFE: return "FAILSAFE";
            case CTRL_MANUAL: return "MANUAL";
            case CTRL_STANDBY: return "STANDBY";
            default: return "UNKNOWN";
        }
    }

    const char* getVentilationReasonName(VentReason value) {
        switch (value) {
            case VENT_DAY_LOCK: return "DAY_LOCK";
            case VENT_NIGHT: return "NIGHT";
            case VENT_BAD_AIR: return "BAD_AIR";
            case VENT_CRITICAL_AIR: return "CRITICAL_AIR";
            case VENT_COOLING: return "COOLING";
            case VENT_OUTSIDE_HOT: return "OUTSIDE_HOT";
            case VENT_OUTSIDE_COLD: return "OUTSIDE_COLD";
            case VENT_OUTSIDE_WET: return "OUTSIDE_WET";
            case VENT_SENSOR_WAIT: return "SENSOR_WAIT";
            case VENT_FORCED: return "FORCED";
            case VENT_MANUAL_OFF: return "MANUAL_OFF";
            case VENT_CYCLE_GUARD: return "CYCLE_GUARD";
            default: return "UNKNOWN";
        }
    }
}

void drawStatusScreen(bool fullRedraw) {
    if (fullRedraw) {
        // Reset caches
        lastControllerState = CTRL_BOOT;
        lastVentReason = VENT_SENSOR_WAIT;
        lastTimeSyncState = -1;
        lastBmeRoom = false; lastBmeOut = false;
        lastMq = false; lastLight = false; lastRelay = false; lastHeap = -1;
        lastScreen = state.currentScreen;

        applyThemeColor();
        tft.drawRect(0, 0, SCREEN_W, SCREEN_H, state.activeThemeColor);
        tft.drawRect(1, 1, SCREEN_W - 2, SCREEN_H - 2, state.activeThemeColor);
        tft.setCursor(10, 10);
        tft.setTextColor(state.activeThemeColor, TFT_BLACK);
        tft.setTextSize(1);
        tft.print("СТАТУС СИСТЕМЫ");

        // Bottom status line
        tft.fillRect(2, SCREEN_H - 15, SCREEN_W - 4, 13, state.activeThemeColor);
        tft.setCursor(5, SCREEN_H - 12);
        tft.setTextColor(TFT_BLACK, state.activeThemeColor);
        tft.print("НАЖАТИЕ: ГЛАВНЫЙ | УДЕРЖАНИЕ: ПИТАНИЕ");
    }

    char buf[80];
    tft.setTextSize(1);

    // Controller State
    if (fullRedraw || state.controllerState != lastControllerState) {
        lastControllerState = state.controllerState;
        snprintf(buf, sizeof(buf), "КОНТРОЛЛЕР: %s", getControllerStateName(lastControllerState));
        printAt(10, 30, buf, TFT_WHITE, TFT_BLACK);
    }

    // Ventilation Reason
    if (fullRedraw || state.ventDecision.reason != lastVentReason) {
        lastVentReason = state.ventDecision.reason;
        snprintf(buf, sizeof(buf), "ПРИЧИНА (ВЕНТ): %s", getVentilationReasonName(lastVentReason));
        printAt(10, 45, buf, TFT_WHITE, TFT_BLACK);
    }

    // Sensor Status Section
    if (fullRedraw) {
        printAt(10, 65, "СТАТУС СЕНСОРОВ:", state.activeThemeColor, TFT_BLACK);
    }
    
    if (fullRedraw || state.bmeRoomOnline != lastBmeRoom || state.bmeOutsideOnline != lastBmeOut || state.mqReady != lastMq || state.lightSensorOnline != lastLight || state.relayModuleOnline != lastRelay) {
        lastBmeRoom = state.bmeRoomOnline; lastBmeOut = state.bmeOutsideOnline; lastMq = state.mqReady;
        lastLight = state.lightSensorOnline; lastRelay = state.relayModuleOnline;

        snprintf(buf, sizeof(buf), " BME(вн):%s  BME(внеш):%s  MQ135:%s", getOnOff(lastBmeRoom), getOnOff(lastBmeOut), getOnOff(lastMq));
        printAt(10, 80, buf, TFT_WHITE, TFT_BLACK);
        snprintf(buf, sizeof(buf), " СВЕТ:%s    РЕЛЕ:%s", getOnOff(lastLight), getOnOff(lastRelay));
        printAt(10, 95, buf, TFT_WHITE, TFT_BLACK);
    }

    // Time Sync Status
    const int timeSyncState = state.timeSynced ? 2 : (state.timeSyncStarted ? 1 : 0);
    if (fullRedraw || timeSyncState != lastTimeSyncState) {
        lastTimeSyncState = timeSyncState;
        const char* timeSyncText = (timeSyncState == 2) ? "OK" : ((timeSyncState == 1) ? "ПОИСК" : "НЕТ");
        snprintf(buf, sizeof(buf), "СИНХР. ВРЕМЕНИ: %s", timeSyncText);
        printAt(10, 115, buf, TFT_WHITE, TFT_BLACK);
    }

    // Memory Status
    const int freeHeap = ESP.getFreeHeap();
    if (fullRedraw || freeHeap != lastHeap) {
        lastHeap = freeHeap;
        snprintf(buf, sizeof(buf), "ПАМЯТЬ (HEAP): %d B", lastHeap);
        printAt(10, 130, buf, TFT_WHITE, TFT_BLACK);
    }

    // Firmware version
    if (fullRedraw) {
        snprintf(buf, sizeof(buf), "ПРОШИВКА: %s", STR_FIRMWARE_VERSION);
        printAt(10, 150, buf, state.activeThemeColor, TFT_BLACK);
    }

    // Debug Values
    if (fullRedraw) {
        snprintf(buf, sizeof(buf), "ДАТЧИКИ: L:%d%% AQI:%d", state.current.lightPercent, state.current.gasAqi);
        printAt(10, 170, buf, TFT_CYAN, TFT_BLACK);
        snprintf(buf, sizeof(buf), "CO2:%d RELAY:%s", state.current.co2, state.relayOutputOn ? "ON" : "OFF");
        printAt(10, 180, buf, TFT_CYAN, TFT_BLACK);
    }
}
