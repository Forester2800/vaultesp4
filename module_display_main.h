#pragma once

// Main screen and forecast screen update functions.

#include "state.h"
#include "config.h"
#include "strings.h"
#include "module_forecast_core.h"

// --- HELPERS ---

namespace {
  void drawTextRight(const char* text, int16_t x, int16_t y, uint16_t color, uint16_t bgColor = BLACK) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.setTextSize(1);
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    printAt(x - w, y, text, color, bgColor);
  }

  void drawTextCentered(const char* text, int16_t x, int16_t y, uint16_t color, uint16_t bgColor = BLACK) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.setTextSize(1);
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    printAt(x - w / 2, y, text, color, bgColor);
  }

  // Forward-declare vault font functions
  extern void vaultDrawText(int16_t x, int16_t y, const char* text, uint16_t fg, uint16_t bg, uint8_t size);
} // namespace

// --- IMPLEMENTATIONS ---

void drawStatusBar() {
  bool bridgeConnected;
  uint32_t frame;
  if (LOCK_STATE()) {
  bridgeConnected = state.pcBridgeConnected;
  frame = state.frameCounter;
  UNLOCK_STATE();
  }

  char timeBuffer[6], dateBuffer[9];
  struct tm timeInfo;

  if (state.timeSynced && getLocalTime(&timeInfo, 50)) {
    strftime(timeBuffer, sizeof(timeBuffer), STR_TIME_FORMAT, &timeInfo);
    strftime(dateBuffer, sizeof(dateBuffer), STR_DATE_FORMAT, &timeInfo);
  } else {
    int minutes = (millis() / 60000UL) % 60;
    int hours = (millis() / 3600000UL) % 24;
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", hours, minutes);
    snprintf(dateBuffer, sizeof(dateBuffer), STR_DATE_UNKNOWN);
  }

  clearField(5, 8, 60, 10);
  printAt(5, 8, timeBuffer, state.activeThemeColor, BLACK);

  clearField(48, 5, 32, 16);
  if (bridgeConnected) {
    tft.drawRect(49, 5, 24, 13, state.activeThemeColor);
    printAt(53, 8, STR_PC_STATUS, state.activeThemeColor, BLACK);
    if ((frame % 2) == 0) tft.fillRect(68, 8, 2, 2, state.activeThemeColor);
  }

  clearField(170, 8, 65, 10);
  drawTextRight(dateBuffer, 235, 8, state.activeThemeColor, BLACK);
}

void updateMainScreen() {
  if (state.firstMainUpdate) drawMainFrame();

  Readings readings;
  VentDecision decision;
  bool ready, alarm;
  copySharedReadings(readings, decision);
  if (LOCK_STATE()) {
  ready = state.mqReady;
  alarm = state.airQualityAlarm;
  UNLOCK_STATE();
  }

  extern float absoluteHumidity(float, float); // from module_environment_logic
  drawStatusBar();
  updateFloatField(readings.tempRoom, 58, 58, "C");
  updateHumidityField(readings.humRoom, absoluteHumidity(readings.tempRoom, readings.humRoom), 58, 76);
  updateIntField(readings.pressureRoomMm, 58, 94, STR_UNIT_MMHG);
  updateGasField(readings, ready, alarm);
  updateFloatField(readings.tempOutside, 58, 173, "C");
  updateHumidityField(readings.humOutside, absoluteHumidity(readings.tempOutside, readings.humOutside), 58, 191);
  updateIntField(readings.pressureOutsideMm, 58, 209, STR_UNIT_MMHG);
  updateLightField(readings);
  updateRelayField(decision);
  drawProgress(297);
  state.firstMainUpdate = false;
}

void updateForecastScreen() {
  if (state.firstForecastUpdate) drawForecastFrame();

  EnhancedForecast enhanced = getEnhancedForecast();
  int trend = (int)roundf(state.forecastTrendMmPerHour);

  char pressureBuffer[40], trendBuffer[48], confidenceBuffer[16], windowBuffer[36], probabilityBuffer[40];

  snprintf(pressureBuffer, sizeof(pressureBuffer), STR_FORECAST_PRESSURE, state.forecastPressureMm);
  snprintf(trendBuffer, sizeof(trendBuffer), STR_FORECAST_TREND, enhanced.trendName, trend);
  snprintf(confidenceBuffer, sizeof(confidenceBuffer), STR_FORECAST_CONFIDENCE, enhanced.confidence);
  snprintf(windowBuffer, sizeof(windowBuffer), STR_FORECAST_WINDOW, enhanced.windowText);
  snprintf(probabilityBuffer, sizeof(probabilityBuffer), STR_FORECAST_PROBABILITY, enhanced.probability, enhanced.confidence);

  clearField(72, 55, 150, 10);
  printAt(72, 55, pressureBuffer, state.activeThemeColor, BLACK);
  clearField(72, 76, 160, 10);
  printAt(72, 76, trendBuffer, abs(trend) > 2 ? WARN : state.activeThemeColor, BLACK);
  clearField(84, 94, 60, 10);
  printAt(84, 94, confidenceBuffer, enhanced.confidence < 35 ? WARN : state.activeThemeColor, BLACK);

  tft.fillRect(11, 155, 218, 110, BLACK);
  tft.drawRect(10, 154, 220, 112, state.activeThemeColor);

  if (enhanced.mode != UNDEFINED) {
    vaultDrawText(18, 166, enhanced.tag, state.activeThemeColor, BLACK, 2);
    vaultDrawText(18, 196, windowBuffer, state.activeThemeColor, BLACK, 1);
    vaultDrawText(18, 214, enhanced.line1, state.activeThemeColor, BLACK, 1);
    vaultDrawText(18, 232, enhanced.line2, state.activeThemeColor, BLACK, 1);
    vaultDrawText(18, 250, probabilityBuffer, state.activeThemeColor, BLACK, 1);
  } else {
    if (!hasValidClock()) {
      vaultDrawText(18, 166, STR_FORECAST_STATUS_WAITING, state.activeThemeColor, BLACK, 2);
      vaultDrawText(18, 202, STR_FORECAST_STATUS_NTP_SYNC, state.activeThemeColor, BLACK, 1);
      vaultDrawText(18, 224, STR_FORECAST_STATUS_HISTORY_INACTIVE, state.activeThemeColor, BLACK, 1);
    } else {
      vaultDrawText(18, 166, STR_FORECAST_STATUS_ACCUMULATING, state.activeThemeColor, BLACK, 2);
      vaultDrawText(18, 202, STR_FORECAST_STATUS_PRESSURE_HISTORY, state.activeThemeColor, BLACK, 1);
      vaultDrawText(18, 224, STR_FORECAST_STATUS_FORECAST_PENDING, state.activeThemeColor, BLACK, 1);
    }
  }

  drawProgress(297);
  state.firstForecastUpdate = false;
}
