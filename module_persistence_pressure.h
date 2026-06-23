#pragma once

// Preferences-backed pressure history persistence.

#include "module_persistence.h" // даёт `extern Preferences prefs`
#include "strings.h"

// --- FORWARD DECLARATIONS ---
bool hasValidClock();

// --- HELPERS ---

namespace {
  uint32_t fnv1aUpdate(uint32_t hash, const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
      hash ^= data[i];
      hash *= 16777619UL;
    }
    return hash;
  }

  uint32_t pressureHistoryChecksum() {
    uint32_t hash = 2166136261UL;
    hash = fnv1aUpdate(hash, (const uint8_t *)&state.pressureHistoryIndex, sizeof(state.pressureHistoryIndex));
    hash = fnv1aUpdate(hash, (const uint8_t *)&state.pressureSampleCount, sizeof(state.pressureSampleCount));
    hash = fnv1aUpdate(hash, (const uint8_t *)state.pressureHistory, sizeof(state.pressureHistory));
    return hash;
  }
} // namespace

// --- IMPLEMENTATIONS ---

void clearPressureHistory() {
  memset(state.pressureHistory, 0, sizeof(state.pressureHistory));
  state.pressureHistoryIndex = 0;
  state.pressureSampleCount = 0;
  state.pressureHistoryDirty = false;
  state.pressureTrendEmaReady = false;
}

int latestPressureHistoryMm() {
  if (state.pressureSampleCount == 0) return 0;
  uint8_t latest = (state.pressureHistoryIndex + PRESSURE_HISTORY_SIZE - 1) % PRESSURE_HISTORY_SIZE;
  return state.pressureHistory[latest].pressureMm;
}

void savePressureHistory() {
  if (!PERSIST_PRESSURE_HISTORY || !state.pressureHistoryDirty) return;
  prefs.begin(STR_PRESSURE_PREFS_NAMESPACE, false);
  prefs.putUChar("idx", state.pressureHistoryIndex);
  prefs.putUChar("cnt", state.pressureSampleCount);
  prefs.putBytes("items", state.pressureHistory, sizeof(state.pressureHistory));
  prefs.putUInt("crc", pressureHistoryChecksum());
  prefs.end();
  state.pressureHistoryDirty = false;
  state.lastPressureSaveMs = millis();
  state.lastSavedPressureMm = latestPressureHistoryMm();
}

void purgeExpiredPressureSamples() {
  if (!hasValidClock() || state.pressureSampleCount == 0) return;
  time_t now = time(nullptr);
  PressureSample kept[PRESSURE_HISTORY_SIZE];
  uint8_t keptCount = 0;
  for (uint8_t i = 0; i < state.pressureSampleCount; i++) {
    uint8_t index = (state.pressureHistoryIndex + PRESSURE_HISTORY_SIZE - state.pressureSampleCount + i) % PRESSURE_HISTORY_SIZE;
    PressureSample sample = state.pressureHistory[index];
    if (sample.timestamp > 0 && (uint32_t)(now - sample.timestamp) <= PRESSURE_MAX_AGE_SEC) {
      kept[keptCount++] = sample;
    }
  }
  memset(state.pressureHistory, 0, sizeof(state.pressureHistory));
  for (uint8_t i = 0; i < keptCount; i++) state.pressureHistory[i] = kept[i];
  state.pressureSampleCount = keptCount;
  state.pressureHistoryIndex = keptCount % PRESSURE_HISTORY_SIZE;
  state.pressureHistoryDirty = true;
}

void loadPressureHistory() {
  clearPressureHistory();
  if (!PERSIST_PRESSURE_HISTORY) return;
  prefs.begin(STR_PRESSURE_PREFS_NAMESPACE, true);
  state.pressureHistoryIndex = prefs.getUChar("idx", 0);
  state.pressureSampleCount = prefs.getUChar("cnt", 0);
  size_t read = prefs.getBytes("items", state.pressureHistory, sizeof(state.pressureHistory));
  uint32_t savedCrc = prefs.getUInt("crc", 0);
  prefs.end();
  if (read != sizeof(state.pressureHistory) || state.pressureSampleCount > PRESSURE_HISTORY_SIZE || pressureHistoryChecksum() != savedCrc) {
    clearPressureHistory();
    return;
  }
  purgeExpiredPressureSamples();
  state.pressureHistoryDirty = false;
}

void updatePressureHistory() {
  if (millis() - state.lastPressureSampleMs < PRESSURE_SAMPLE_MS) return;
  state.lastPressureSampleMs = millis();
  time_t now = hasValidClock() ? time(nullptr) : (time_t)(millis() / 1000UL);
  int pressure = state.current.pressureOutsideMm;
  if (!USE_MOCK_SENSORS && !state.bmeOutsideOnline) return;
  if (pressure <= 0) return;
  state.pressureHistory[state.pressureHistoryIndex] = {now, pressure};
  state.pressureHistoryIndex = (state.pressureHistoryIndex + 1) % PRESSURE_HISTORY_SIZE;
  if (state.pressureSampleCount < PRESSURE_HISTORY_SIZE) state.pressureSampleCount++;
  state.pressureHistoryDirty = true;
  if (state.pressureSampleCount >= PRESSURE_HISTORY_PURGE_THRESHOLD) purgeExpiredPressureSamples();
  bool firstSave = state.lastSavedPressureMm < 0;
  bool saveIntervalReady = millis() - state.lastPressureSaveMs >= PRESSURE_SAVE_MIN_INTERVAL_MS;
  bool periodicSave = millis() - state.lastPressureSaveMs >= PRESSURE_SAVE_PERIOD_MS;
  bool significantMove = abs(pressure - state.lastSavedPressureMm) >= PRESSURE_SAVE_DELTA_MM;
  if (firstSave || (saveIntervalReady && (periodicSave || significantMove))) {
    savePressureHistory();
  }
}
