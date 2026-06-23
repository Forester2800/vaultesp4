#pragma once

// Core ventilation decision logic.

#include "state.h"
#include "config.h"
#include "module_settings.h"

VentDecision decideVentilation() {
  bool manualModeActive = false;
  bool manualVentMode = false;
  bool manualRelayOn = false;
  bool safetyFaultActive = true;
  bool bmeRoomOnline = false;
  bool bmeOutsideOnline = false;
  bool mqReady = false;
  bool airQualityCritical = false;
  bool airQualityAlarm = false;
  bool coolingLatch = false;
  bool nightMode = false;
  bool outsideAirDry = false;
  bool outsideTempFallingFast = false;
  Readings current = {};

  if (LOCK_STATE()) {
    manualModeActive = state.manualModeActive;
    manualVentMode = state.manualVentMode;
    manualRelayOn = state.manualRelayOn;
    safetyFaultActive = state.safetyFaultActive;
    bmeRoomOnline = state.bmeRoomOnline;
    bmeOutsideOnline = state.bmeOutsideOnline;
    mqReady = state.mqReady;
    airQualityCritical = state.airQualityCritical;
    airQualityAlarm = state.airQualityAlarm;
    coolingLatch = state.coolingLatch;
    nightMode = state.nightMode;
    outsideAirDry = state.outsideAirDry;
    outsideTempFallingFast = state.outsideTempFallingFast;
    current = state.current;
    UNLOCK_STATE();
  } else {
    return {false, VENT_SENSOR_WAIT};
  }

  // Жесткий приоритет: если тумблер режима в положении Ручной, блокируем автоматику
  if (manualModeActive) {
    return {manualRelayOn, manualRelayOn ? VENT_FORCED : VENT_MANUAL_OFF};
  }

  // Проверка ручного режима от сервера (для совместимости)
  if (manualVentMode) {
    return {manualRelayOn, manualRelayOn ? VENT_FORCED : VENT_MANUAL_OFF};
  }

  if (safetyFaultActive) return {false, VENT_SENSOR_WAIT}; // Changed from VENT_FAILSAFE
  if (!bmeRoomOnline || !bmeOutsideOnline || !mqReady) return {false, VENT_SENSOR_WAIT};

  if (airQualityCritical) return {true, VENT_CRITICAL_AIR};
  if (airQualityAlarm) return {true, VENT_BAD_AIR};

  if (current.tempOutside > settings.outsideSafeMax) return {false, VENT_OUTSIDE_HOT};
  if (current.tempOutside < settings.outsideSafeMin) return {false, VENT_OUTSIDE_COLD};

  if (current.humOutside > settings.outsideHumidityLock) return {false, VENT_OUTSIDE_WET};

  if (coolingLatch) return {true, VENT_COOLING};

  if (nightMode) {
    if (outsideAirDry) return {true, VENT_NIGHT};
    if (outsideTempFallingFast && current.tempRoom > PREDICTIVE_ROOM_HOLD_C) return {true, VENT_NIGHT};
  }

  return {false, VENT_DAY_LOCK};
}

VentDecision applyRelayCycleProtection(const VentDecision &decision) {
  bool bypass = false;
  bool relayOutputOn = false;
  uint32_t relayStateChangedMs = 0;
  if (LOCK_STATE()) {
    bypass = state.relayGuardBypassOnce;
    if (bypass) state.relayGuardBypassOnce = false; // Consume the bypass flag
    relayOutputOn = state.relayOutputOn;
    relayStateChangedMs = state.relayStateChangedMs;
    UNLOCK_STATE();
  } else {
    return {false, VENT_SENSOR_WAIT};
  }
  
  if (bypass) return decision;

  bool relayChangeRequested = decision.relayOn != relayOutputOn;
  bool isProtected = (millis() - relayStateChangedMs) < settings.relayMinSwitchMs;

  if (relayChangeRequested && isProtected) {
    return {relayOutputOn, VENT_CYCLE_GUARD};
  }

  return decision;
}
