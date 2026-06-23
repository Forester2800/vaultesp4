#pragma once

// Wi-Fi/NTP time synchronization.

#include "state.h"
#include "config.h"
#include "credentials.h"
#include <WiFi.h>
#include "time.h"

// --- FORWARD DECLARATIONS ---
void loadPressureHistory();
void loadMqBurnInState();

// --- HELPERS ---

namespace {
  void stopTimeSyncWifi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    if (LOCK_STATE()) {
    state.timeSyncStarted = false;
    state.ntpConfigured = false;
    UNLOCK_STATE();
    }
  }

  void scheduleTimeSyncRetry() {
    stopTimeSyncWifi();
    Serial.println("Предупреждение: Wi-Fi не доступен, работаем автономно через USB");
    if (LOCK_STATE()) {
    state.lastTimeSyncAttemptMs = millis();
    uint32_t nextBackoffMs = state.timeSyncBackoffMs * 2UL;
    state.timeSyncBackoffMs = nextBackoffMs > TIME_SYNC_MAX_RETRY_MS ? TIME_SYNC_MAX_RETRY_MS : nextBackoffMs;
    UNLOCK_STATE();
    }
  }

  void startTimeSync() {
    if (!USE_WIFI_TIME) return;
    if (LOCK_STATE()) {
    state.lastTimeSyncAttemptMs = millis();
    state.timeSyncStarted = true;
    state.ntpConfigured = false;
    state.timeSynced = false;
    UNLOCK_STATE();
    }
    // Очистка старых настроек Wi-Fi для надежного подключения
    WiFi.disconnect(true, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Попытка подключения к Wi-Fi...");
  }

  void pollTimeSync() {
    bool timeSynced, timeSyncStarted;
    if (LOCK_STATE()) {
    timeSynced = state.timeSynced;
    timeSyncStarted = state.timeSyncStarted;
    UNLOCK_STATE();
    }
    
    if (!USE_WIFI_TIME || timeSynced || !timeSyncStarted) return;
    
    uint32_t lastTimeSyncAttemptMs;
    if (LOCK_STATE()) {
    lastTimeSyncAttemptMs = state.lastTimeSyncAttemptMs;
    UNLOCK_STATE();
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      if (millis() - lastTimeSyncAttemptMs >= TIME_SYNC_CONNECT_TIMEOUT_MS) scheduleTimeSyncRetry();
      return;
    }
    
    bool ntpConfigured;
    if (LOCK_STATE()) {
    ntpConfigured = state.ntpConfigured;
    UNLOCK_STATE();
    }
    
    if (!ntpConfigured) {
      configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
      if (LOCK_STATE()) {
      state.ntpConfigured = true;
      state.ntpConfiguredMs = millis();
      UNLOCK_STATE();
      }
    }
    struct tm timeInfo;
    bool synced = getLocalTime(&timeInfo, 100);
    
    if (LOCK_STATE()) {
    state.timeSynced = synced;
    UNLOCK_STATE();
    }
    
    if (synced) {
      char timeStr[64];
      strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeInfo);
      Serial.printf("Время синхронизировано: %s\n", timeStr);
      stopTimeSyncWifi();
      if (LOCK_STATE()) {
      state.timeSyncBackoffMs = TIME_SYNC_INITIAL_RETRY_MS;
      UNLOCK_STATE();
      }
      return;
    }
    
    uint32_t ntpConfiguredMs;
    if (LOCK_STATE()) {
    ntpConfiguredMs = state.ntpConfiguredMs;
    UNLOCK_STATE();
    }
    
    if (millis() - ntpConfiguredMs > TIME_SYNC_NTP_TIMEOUT_MS) scheduleTimeSyncRetry();
  }

} // namespace

// --- PUBLIC API ---

bool hasValidClock() {
  bool synced;
  if (LOCK_STATE()) {
  synced = state.timeSynced;
  UNLOCK_STATE();
  }
  return synced || (millis() / 1000UL) >= MIN_UPTIME_FOR_VALID_CLOCK_SEC;
}

bool timestampHasValidClock(time_t timestamp) {
  return timestamp > MIN_VALID_EPOCH_TIME;
}

void maintainTimeSync() {
  if (!USE_WIFI_TIME) return;
  bool wasSynced, timeSynced, timeSyncStarted;
  uint32_t lastTimeSyncAttemptMs, timeSyncBackoffMs;
  
  if (LOCK_STATE()) {
  wasSynced = state.timeSynced;
  UNLOCK_STATE();
  }
  
  pollTimeSync();
  
  if (LOCK_STATE()) {
  timeSynced = state.timeSynced;
  UNLOCK_STATE();
  }
  
  if (timeSynced && !wasSynced) {
    loadPressureHistory();
    loadMqBurnInState();
  }
  
  if (LOCK_STATE()) {
  timeSynced = state.timeSynced;
  timeSyncStarted = state.timeSyncStarted;
  lastTimeSyncAttemptMs = state.lastTimeSyncAttemptMs;
  timeSyncBackoffMs = state.timeSyncBackoffMs;
  UNLOCK_STATE();
  }
  
  if (!timeSynced && !timeSyncStarted && millis() - lastTimeSyncAttemptMs >= timeSyncBackoffMs) {
    startTimeSync();
  }
}
