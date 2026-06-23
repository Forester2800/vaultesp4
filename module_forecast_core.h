#pragma once

// Pressure history analysis and weather forecasting logic.

#include "state.h"
#include "config.h"
#include "strings.h"

// --- FORWARD DECLARATIONS ---
int latestPressureHistoryMm(); // From persistence
float absoluteHumidity(float, float); // From environment logic

// --- HELPER FUNCTIONS ---

namespace {
  uint32_t timeDeltaAbsSec(time_t a, time_t b) {
    return a >= b ? (uint32_t)(a - b) : (uint32_t)(b - a);
  }

  uint8_t minScore3(uint8_t a, uint8_t b, uint8_t c) {
    return min(a, min(b, c));
  }

  uint8_t scoreFromRange(uint32_t value, uint32_t minValue, uint32_t fullValue, uint8_t minScore) {
    if (fullValue <= minValue) return value >= minValue ? 100 : 0;
    if (value <= minValue) return minScore;
    if (value >= fullValue) return 100;
    uint32_t scaled = ((value - minValue) * (100UL - minScore)) / (fullValue - minValue);
    return (uint8_t)constrain((uint32_t)minScore + scaled, (uint32_t)minScore, 100UL);
  }

  uint8_t expectedPressureSamplesForWindow(uint32_t maxAgeSec) {
    uint32_t sampleSec = max(1UL, PRESSURE_SAMPLE_MS / 1000UL);
    uint32_t expected = maxAgeSec / sampleSec + 1UL;
    expected = constrain(expected, (uint32_t)PRESSURE_REGRESSION_MIN_SAMPLES, (uint32_t)PRESSURE_HISTORY_SIZE);
    return (uint8_t)expected;
  }

  uint8_t trendConsistencyScore(PressureTrendResult shortTrend, PressureTrendResult mediumTrend, PressureTrendResult longTrend) {
    if (!mediumTrend.ready) return 0;
    if (!shortTrend.ready && !longTrend.ready) return mediumTrend.confidence;
    
    uint8_t directionScore = 100;
    uint8_t magnitudeScore = 100;
    
    // Check direction consistency
    bool mediumFalling = mediumTrend.trendMmPerHour < -0.3f;
    bool mediumRising = mediumTrend.trendMmPerHour > 0.3f;
    
    if (shortTrend.ready) {
      bool shortFalling = shortTrend.trendMmPerHour < -0.3f;
      bool shortRising = shortTrend.trendMmPerHour > 0.3f;
      
      if (mediumFalling && !shortFalling) directionScore -= 30;
      if (mediumRising && !shortRising) directionScore -= 30;
      if (!mediumFalling && !mediumRising && (shortFalling || shortRising)) directionScore -= 20;
    }
    
    if (longTrend.ready) {
      bool longFalling = longTrend.trendMmPerHour < -0.3f;
      bool longRising = longTrend.trendMmPerHour > 0.3f;
      
      if (mediumFalling && !longFalling) directionScore -= 20;
      if (mediumRising && !longRising) directionScore -= 20;
    }
    
    // Check magnitude consistency
    float mediumMag = fabs(mediumTrend.trendMmPerHour);
    if (shortTrend.ready) {
      float shortMag = fabs(shortTrend.trendMmPerHour);
      float magRatio = mediumMag > 0.1f ? shortMag / mediumMag : 0.0f;
      if (magRatio < 0.5f || magRatio > 2.0f) magnitudeScore -= 20;
    }
    
    // Weight confidence by individual trend confidences
    uint8_t avgConfidence = mediumTrend.confidence;
    if (shortTrend.ready) avgConfidence = (avgConfidence + shortTrend.confidence) / 2;
    if (longTrend.ready) avgConfidence = (avgConfidence + longTrend.confidence) / 2;
    
    uint8_t finalScore = (directionScore * 60 + magnitudeScore * 20 + avgConfidence * 20) / 100;
    return constrain(finalScore, (uint8_t)0, (uint8_t)100);
  }
} // namespace

// --- CORE FORECASTING LOGIC ---

PressureTrendResult analyzePressureTrend(uint32_t maxAgeSec, bool updateEma) {
  PressureTrendResult result = {false, 0.0f, 0, 0, 0, 0, 0, 0};
  if (state.pressureSampleCount < PRESSURE_REGRESSION_MIN_SAMPLES) return result;
  time_t newest = 0;
  for (uint8_t i = 0; i < state.pressureSampleCount; i++) {
    uint8_t index = (state.pressureHistoryIndex + PRESSURE_HISTORY_SIZE - state.pressureSampleCount + i) % PRESSURE_HISTORY_SIZE;
    newest = max(newest, state.pressureHistory[index].timestamp);
  }
  if (newest == 0) return result;

  double sw = 0, sx = 0, sy = 0, sxx = 0, sxy = 0, syy = 0;
  uint8_t count = 0;
  uint32_t oldestAge = 0;
  for (uint8_t i = 0; i < state.pressureSampleCount; i++) {
    uint8_t index = (state.pressureHistoryIndex + PRESSURE_HISTORY_SIZE - state.pressureSampleCount + i) % PRESSURE_HISTORY_SIZE;
    PressureSample sample = state.pressureHistory[index];
    if (sample.timestamp == 0 || sample.timestamp > newest) continue;
    uint32_t age = (uint32_t)(newest - sample.timestamp);
    if (age > maxAgeSec) continue;
    double x = -((double)age) / 3600.0;
    double y = sample.pressureMm;
    double recent = maxAgeSec == 0 ? 1.0 : 1.0 - ((double)age / (double)maxAgeSec);
    double w = 0.55 + recent * 0.45;
    sw += w; sx += w * x; sy += w * y; sxx += w * x * x; sxy += w * x * y; syy += w * y * y;
    oldestAge = max(oldestAge, age);
    count++;
  }
  if (count < PRESSURE_REGRESSION_MIN_SAMPLES) return result;
  double xVar = sw * sxx - sx * sx;
  double yVar = sw * syy - sy * sy;
  double covariance = sw * sxy - sx * sy;
  if (fabs(xVar) < 0.0001) return result;
  double slope = covariance / xVar;
  double rDenom = xVar * yVar;
  double r2;
  if (fabs(yVar) < 0.0001) r2 = fabs(slope) < 0.05 ? 1.0 : 0.0;
  else r2 = rDenom > 0 ? (covariance * covariance) / rDenom : 0.0;
  r2 = constrain(r2, 0.0, 1.0);

  result.ready = true;
  result.trendMmPerHour = (float)slope;
  result.sampleCount = count;
  result.spanSec = oldestAge;
  result.r2Score = (uint8_t)round(r2 * 100.0);
  uint8_t expectedSamples = expectedPressureSamplesForWindow(maxAgeSec);
  result.countScore = scoreFromRange(count, PRESSURE_REGRESSION_MIN_SAMPLES, expectedSamples, 35);
  uint32_t targetSpan = maxAgeSec > PRESSURE_TREND_SPAN_TOLERANCE_SEC ? maxAgeSec - PRESSURE_TREND_SPAN_TOLERANCE_SEC : maxAgeSec;
  result.spacingScore = scoreFromRange(oldestAge, 0, targetSpan, 0);
  result.confidence = minScore3(result.r2Score, result.countScore, result.spacingScore);

  if (updateEma) {
    bool pressureTrendEmaReady;
    float pressureTrendEma;
    if (LOCK_STATE()) {
    pressureTrendEmaReady = state.pressureTrendEmaReady;
    pressureTrendEma = state.pressureTrendEma;
    UNLOCK_STATE();
    }
    
    if (!pressureTrendEmaReady) {
      if (LOCK_STATE()) {
      state.pressureTrendEma = result.trendMmPerHour;
      UNLOCK_STATE();
      }
    } else {
      float newEma = result.trendMmPerHour * PRESSURE_TREND_EMA_ALPHA + pressureTrendEma * (1.0f - PRESSURE_TREND_EMA_ALPHA);
      if (LOCK_STATE()) {
      state.pressureTrendEma = newEma;
      UNLOCK_STATE();
      }
    }
    
    if (LOCK_STATE()) {
    state.pressureTrendEmaReady = true;
    state.pressureTrendConfidence = result.confidence;
    state.pressureTrendEmaSourceTs = newest;
    pressureTrendEma = state.pressureTrendEma;
    UNLOCK_STATE();
    }
    
    result.trendMmPerHour = pressureTrendEma;
  }
  return result;
}

bool forecastReady() {
  PressureTrendResult result = analyzePressureTrend(PRESSURE_TREND_MEDIUM_SEC, false);
  return result.ready && result.confidence >= FORECAST_MIN_CONFIDENCE;
}

EnhancedForecast getEnhancedForecast() {
  PressureTrendResult shortTrend = analyzePressureTrend(PRESSURE_TREND_SHORT_SEC, false);
  PressureTrendResult mediumTrend = analyzePressureTrend(PRESSURE_TREND_MEDIUM_SEC, true);
  PressureTrendResult longTrend = analyzePressureTrend(PRESSURE_TREND_LONG_SEC, false);

  Readings current;
  if (LOCK_STATE()) {
  current = state.current;
  UNLOCK_STATE();
  }
  
  int currentPressure = (current.pressureOutsideMm > 0) ? current.pressureOutsideMm : latestPressureHistoryMm();
  
  if (LOCK_STATE()) {
  state.forecastPressureMm = (currentPressure > 0) ? currentPressure : 0;
  state.forecastTrendMmPerHour = 0.0f;
  UNLOCK_STATE();
  }

  auto makeForecast = [](EnhancedForecastMode mode, const char *tag, const char *window, const char *l1, const char *l2, uint8_t prob, uint8_t conf, const char *trend) {
    return EnhancedForecast{mode, tag, trend, window, l1, l2, prob, conf};
  };

  if (!mediumTrend.ready) {
    if (LOCK_STATE()) {
    state.pressureTrendConfidence = 0;
    UNLOCK_STATE();
    }
    return makeForecast(UNDEFINED, STR_ANALYSIS_ACCUM, STR_FORECAST_UNDEFINED, STR_FORECAST_STATUS_PRESSURE_HISTORY, STR_FORECAST_ACCUMULATING_DETAILS, 0, 0, STR_TREND_STABLE);
  }

  float trend = mediumTrend.trendMmPerHour;
  
  if (LOCK_STATE()) {
  state.forecastTrendMmPerHour = trend;
  UNLOCK_STATE();
  }
  
  float acceleration = shortTrend.ready ? shortTrend.trendMmPerHour - mediumTrend.trendMmPerHour : 0.0f;
  uint8_t consistency = trendConsistencyScore(shortTrend, mediumTrend, longTrend);
  uint8_t confidence = min(mediumTrend.confidence, consistency);
  
  if (LOCK_STATE()) {
  state.pressureTrendConfidence = confidence;
  UNLOCK_STATE();
  }

  if (confidence < FORECAST_MIN_CONFIDENCE) {
    return makeForecast(UNDEFINED, STR_ANALYSIS_ACCUM, STR_FORECAST_UNDEFINED, STR_FORECAST_STATUS_PRESSURE_HISTORY, STR_FORECAST_ACCUMULATING_DETAILS, 0, confidence, STR_TREND_STABLE);
  }

  int forecastPressureMm;
  if (LOCK_STATE()) {
  forecastPressureMm = state.forecastPressureMm;
  UNLOCK_STATE();
  }
  
  uint8_t pressureBonus = (forecastPressureMm <= 740) ? 20 : ((forecastPressureMm <= 745) ? 14 : ((forecastPressureMm <= 750) ? 7 : ((forecastPressureMm >= 762) ? 0 : 3)));
  int probability = 10 + pressureBonus;
  if (trend <= -3.5f) probability = 78; else if (trend <= -2.0f) probability = 64; else if (trend <= -0.8f) probability = 46; else if (trend < -0.3f) probability = 32; else if (trend >= 1.0f) probability = 12; else probability = 18;

  if (trend < -0.8f) {
    probability += pressureBonus;
    
    Readings current;
    if (LOCK_STATE()) {
    current = state.current;
    UNLOCK_STATE();
    }
    
    float outsideAh = absoluteHumidity(current.tempOutside, current.humOutside);
    if (isfinite(outsideAh)) {
      if (outsideAh >= 12.0f) probability += 18; else if (outsideAh >= 9.0f) probability += 10;
    }
    if (acceleration <= -0.8f) probability += 12; else if (acceleration >= 0.8f) probability -= 8;
  }
  probability = constrain(probability, 0, 100);

  if (LOCK_STATE()) {
  forecastPressureMm = state.forecastPressureMm;
  UNLOCK_STATE();
  }
  
  bool persistentFront = longTrend.ready && longTrend.trendMmPerHour <= -1.1f && mediumTrend.trendMmPerHour <= -1.1f && forecastPressureMm > 0 && forecastPressureMm <= 750;

  const char* trendName = (trend > 1.0f) ? STR_TREND_RISING : ((trend < -1.0f) ? STR_TREND_FALLING : STR_TREND_STABLE);

  if (trend <= -3.5f) return makeForecast(PRECIP_SOON, STR_FORECAST_TAG_SOON, STR_FORECAST_WINDOW_1_3H, STR_FORECAST_LINE1_PRECIP_PROBABLE, STR_FORECAST_LINE2_FRONT_APPROACHING, probability, confidence, trendName);
  if (trend <= -2.0f) return makeForecast(PRECIP_ATTENTION, STR_FORECAST_TAG_ATTENTION, STR_FORECAST_WINDOW_3_6H, STR_FORECAST_LINE1_PRECIP_POSSIBLE, STR_FORECAST_LINE2_PRESSURE_FALLING, probability, confidence, trendName);
  if (persistentFront) return makeForecast(FRONT_PERSISTENT, STR_FORECAST_TAG_FRONT, STR_FORECAST_WINDOW_6_12H, STR_FORECAST_LINE1_BAD_WEATHER_MAY, STR_FORECAST_LINE2_PERSIST, probability, confidence, trendName);
  if (trend <= -0.8f || (trend < -0.3f && confidence < 45)) return makeForecast(PRECIP_WATCH, STR_FORECAST_TAG_WATCH, STR_FORECAST_WINDOW_6_12H, STR_FORECAST_LINE1_PRECIP_RISK, STR_FORECAST_LINE2_NEEDS_MONITORING, probability, confidence, trendName);
  if (trend >= 0.8f) return makeForecast(CLEARING, STR_FORECAST_TAG_CLEARING, STR_FORECAST_WINDOW_2_6H, STR_FORECAST_LINE1_CONDITIONS_IMPROVING, STR_FORECAST_LINE2_FRONT_WEAKENING, probability, confidence, trendName);
  return makeForecast(STABLE, STR_FORECAST_TAG_STABLE, STR_FORECAST_WINDOW_12H, STR_FORECAST_LINE1_NO_SIGNIFICANT, STR_FORECAST_LINE2_CHANGES, probability, confidence, trendName);
}
