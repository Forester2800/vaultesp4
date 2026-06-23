#pragma once

// Manages device configuration settings that can be changed at runtime.

#include "config.h"
#include "state.h"

// Structure to hold all runtime-configurable settings.
struct Settings {
  // Sensor calibration
  float altitude;
  float tempOffsetRoom;
  float tempOffsetOutside;

  // Ventilation thresholds
  float roomHotOn;
  float roomCoolOff;
  float outsideComfortMin;
  float outsideComfortMax;
  float outsideSafeMin;
  float outsideSafeMax;
  float outsideEmergencyMin;
  float outsideEmergencyMax;
  float coolingDeltaC;
  float outsideHumidityLock;
  float absHumidityDryOnDelta;
  float absHumidityDryOffDelta;
  
  // Gas & Light sensor thresholds
  int mqBadOnAqi;
  int mqGoodOffAqi;
  int mqCriticalOnAqi;
  int lightNightOn;
  int lightDayOff;
  
  // Timing
  uint32_t manualVentTimeoutMin;
  uint32_t relayMinSwitchMs;
};

extern Settings settings;

// Loads settings from Preferences, applying defaults if not found.
void loadSettings();

// Saves the current settings to Preferences.
void saveSettings();
