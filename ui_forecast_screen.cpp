
#include "ui_forecast_screen.h"
#include "state.h"
#include "module_ui_helpers.h"
#include "config.h"
#include "module_forecast_core.h" // <<< ИСПРАВЛЕНО

namespace {
    // --- Cached values to avoid unnecessary redraws ---
    float lastForecastTrend = -999;
    int lastForecastPressure = -1;
    EnhancedForecastMode lastForecastMode = UNDEFINED;
    uint8_t lastForecastConfidence = 255;
    uint8_t lastForecastProbability = 255;
    uint8_t lastScreen = -1;

    // --- Helper to draw the pressure history graph ---
    void drawPressureHistoryGraph(bool redraw) {
        const int16_t x = 8, y = 50, w = 224, h = 90;
        if (redraw) {
            tft.drawRect(x, y, w, h, state.activeThemeColor);
        }

        LOCK_STATE();
        // Find min/max pressure in history
        int minP = 1000, maxP = 0;
        for (int i = 0; i < state.pressureSampleCount; ++i) {
            if (state.pressureHistory[i].pressureMm < minP) minP = state.pressureHistory[i].pressureMm;
            if (state.pressureHistory[i].pressureMm > maxP) maxP = state.pressureHistory[i].pressureMm;
        }
        UNLOCK_STATE();

        if (maxP - minP < 4) { // Ensure a minimum span for visibility
            minP -= 2;
            maxP += 2;
        }

        tft.fillRect(x + 1, y + 1, w - 2, h - 2, TFT_BLACK); // Clear graph area

        LOCK_STATE();
        for (int i = 0; i < state.pressureSampleCount; ++i) {
            int p_idx = (state.pressureHistoryIndex + i) % PRESSURE_HISTORY_SIZE;
            if (state.pressureHistory[p_idx].timestamp == 0) continue;

            int16_t px = x + w - 2 - map(i, 0, PRESSURE_HISTORY_SIZE - 1, 0, w - 4);
            int16_t py = y + h - 2 - map(state.pressureHistory[p_idx].pressureMm, minP, maxP, 2, h - 4);
            tft.drawPixel(px, py, state.activeThemeColor);
            tft.drawPixel(px+1, py, state.activeThemeColor);
        }
        UNLOCK_STATE();
    }
}

void drawForecastScreen(bool fullRedraw) {
    EnhancedForecast forecast = getEnhancedForecast();

    if (fullRedraw) {
        // Reset caches
        lastForecastTrend = -999; lastForecastPressure = -1; 
        lastForecastMode = UNDEFINED; lastForecastConfidence = 255; lastForecastProbability = 255;
        lastScreen = state.currentScreen;

        applyThemeColor();
        tft.drawRect(0, 0, SCREEN_W, SCREEN_H, state.activeThemeColor);
        tft.drawRect(1, 1, SCREEN_W - 2, SCREEN_H - 2, state.activeThemeColor);
        tft.setCursor(10, 10);
        tft.setTextColor(state.activeThemeColor, TFT_BLACK);
        tft.setTextSize(1);
        tft.print("ПРОГНОЗ - АНАЛИЗ ДАВЛЕНИЯ");

        // Bottom status line
        tft.fillRect(2, SCREEN_H - 15, SCREEN_W - 4, 13, state.activeThemeColor);
        tft.setCursor(5, SCREEN_H - 12);
        tft.setTextColor(TFT_BLACK, state.activeThemeColor);
        tft.print("НАЖАТИЕ: СТАТУС | УДЕРЖАНИЕ: ПИТАНИЕ");

        drawPressureHistoryGraph(true);
    }

    // --- Update dynamic data ---
    char buf[64];

    // Forecast text lines
    if (fullRedraw || forecast.mode != lastForecastMode) {
        lastForecastMode = forecast.mode;
        tft.fillRect(8, 25, 224, 20, TFT_BLACK);
        tft.setTextColor(state.activeThemeColor, TFT_BLACK);
        tft.setTextSize(1);
        tft.setCursor(10, 26);
        tft.print(forecast.line1);
        tft.setCursor(10, 36);
        tft.print(forecast.line2);
    }

    // Graph needs redraw on every frame for now
    drawPressureHistoryGraph(false);

    // Trend data
    if (fullRedraw || state.forecastTrendMmPerHour != lastForecastTrend || state.forecastPressureMm != lastForecastPressure) {
        lastForecastTrend = state.forecastTrendMmPerHour;
        lastForecastPressure = state.forecastPressureMm;
        
        tft.fillRect(8, 145, 224, 24, TFT_BLACK);
        tft.setTextColor(state.activeThemeColor, TFT_BLACK);
        tft.setTextSize(1);

        snprintf(buf, sizeof(buf), "ТРЕНД: %.2f мм/ч (%s)", lastForecastTrend, forecast.trendName);
        tft.setCursor(10, 148);
        tft.print(buf);

        snprintf(buf, sizeof(buf), "ДАВЛЕНИЕ: %d мм", lastForecastPressure);
        tft.setCursor(10, 158);
        tft.print(buf);
    }

    // Confidence/Probability
    if (fullRedraw || forecast.confidence != lastForecastConfidence || forecast.probability != lastForecastProbability) {
        lastForecastConfidence = forecast.confidence;
        lastForecastProbability = forecast.probability;

        tft.fillRect(8, 175, 224, 30, TFT_BLACK);
        tft.setTextColor(state.activeThemeColor, TFT_BLACK);
        tft.setTextSize(1);

        snprintf(buf, sizeof(buf), "УВЕРЕННОСТЬ: %d%%", lastForecastConfidence);
        tft.setCursor(10, 178);
        tft.print(buf);

        snprintf(buf, sizeof(buf), "ВЕРОЯТНОСТЬ: %d%% (%s)", lastForecastProbability, forecast.tag);
        tft.setCursor(10, 188);
        tft.print(buf);
    }
}
