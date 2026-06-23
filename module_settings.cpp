#include "module_settings.h"
#include <Preferences.h>

Settings settings;

namespace {
  Preferences prefs;
  const char* SETTINGS_NAMESPACE = "settings";
}

void loadSettings() {
  prefs.begin(SETTINGS_NAMESPACE, false);

  // Sensor calibration
  settings.altitude = prefs.getFloat("altitude", ALTITUDE_METERS);
  settings.tempOffsetRoom = prefs.getFloat("tempOffsetRoom", 0.0f);
  settings.tempOffsetOutside = prefs.getFloat("tempOffsetOutside", 0.0f);
  
  // Ventilation thresholds
  settings.roomHotOn = prefs.getFloat("roomHotOn", ROOM_HOT_ON);
  settings.roomCoolOff = prefs.getFloat("roomCoolOff", ROOM_COOL_OFF);
  settings.outsideComfortMin = prefs.getFloat("outsideComfortMin", OUTSIDE_COMFORT_MIN);
  settings.outsideComfortMax = prefs.getFloat("outsideComfortMax", OUTSIDE_COMFORT_MAX);
  settings.outsideSafeMin = prefs.getFloat("outsideSafeMin", OUTSIDE_SAFE_MIN);
  settings.outsideSafeMax = prefs.getFloat("outsideSafeMax", OUTSIDE_SAFE_MAX);
  settings.outsideEmergencyMin = prefs.getFloat("outsideEmergencyMin", OUTSIDE_EMERGENCY_MIN);
  settings.outsideEmergencyMax = prefs.getFloat("outsideEmergencyMax", OUTSIDE_EMERGENCY_MAX);
  settings.coolingDeltaC = prefs.getFloat("coolingDeltaC", COOLING_DELTA_C);
  settings.outsideHumidityLock = prefs.getFloat("outsideHumidityLock", OUTSIDE_HUMIDITY_LOCK);
  settings.absHumidityDryOnDelta = prefs.getFloat("absHumDryOnDelta", ABS_HUMIDITY_DRY_ON_DELTA);
  settings.absHumidityDryOffDelta = prefs.getFloat("absHumDryOffDelta", ABS_HUMIDITY_DRY_OFF_DELTA);

  // Gas & Light sensor thresholds
  settings.mqBadOnAqi = prefs.getInt("mqBadOnAqi", MQ_BAD_ON);
  settings.mqGoodOffAqi = prefs.getInt("mqGoodOffAqi", MQ_GOOD_OFF);
  settings.mqCriticalOnAqi = prefs.getInt("mqCriticalOnAqi", MQ_CRITICAL_ON);
  settings.lightNightOn = prefs.getInt("lightNightOn", LIGHT_NIGHT_ON);
  settings.lightDayOff = prefs.getInt("lightDayOff", LIGHT_DAY_OFF);

  // Timing
  settings.manualVentTimeoutMin = prefs.getUInt("manualVentTimeout", 15);
  settings.relayMinSwitchMs = prefs.getUInt("relayMinSwitchMs", RELAY_MIN_SWITCH_MS);

  prefs.end();
}

void saveSettings() {
  prefs.begin(SETTINGS_NAMESPACE, false);
  
  prefs.putFloat("altitude", settings.altitude);
  prefs.putFloat("tempOffsetRoom", settings.tempOffsetRoom);
  prefs.putFloat("tempOffsetOutside", settings.tempOffsetOutside);
  
  prefs.putFloat("roomHotOn", settings.roomHotOn);
  prefs.putFloat("roomCoolOff", settings.roomCoolOff);
  prefs.putFloat("outsideComfortMin", settings.outsideComfortMin);
  prefs.putFloat("outsideComfortMax", settings.outsideComfortMax);
  prefs.putFloat("outsideSafeMin", settings.outsideSafeMin);
  prefs.putFloat("outsideSafeMax", settings.outsideSafeMax);
  prefs.putFloat("outsideEmergencyMin", settings.outsideEmergencyMin);
  prefs.putFloat("outsideEmergencyMax", settings.outsideEmergencyMax);
  prefs.putFloat("coolingDeltaC", settings.coolingDeltaC);
  prefs.putFloat("outsideHumidityLock", settings.outsideHumidityLock);
  prefs.putFloat("absHumDryOnDelta", settings.absHumidityDryOnDelta);
  prefs.putFloat("absHumDryOffDelta", settings.absHumidityDryOffDelta);
  
  prefs.putInt("mqBadOnAqi", settings.mqBadOnAqi);
  prefs.putInt("mqGoodOffAqi", settings.mqGoodOffAqi);
  prefs.putInt("mqCriticalOnAqi", settings.mqCriticalOnAqi);
  prefs.putInt("lightNightOn", settings.lightNightOn);
  prefs.putInt("lightDayOff", settings.lightDayOff);
  
  prefs.putUInt("manualVentTimeout", settings.manualVentTimeoutMin);
  prefs.putUInt("relayMinSwitchMs", settings.relayMinSwitchMs);
  
  prefs.end();
}
