#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "Readings.h"
#include "config.h"

// --- Enums for State Management ---

enum FaultCode : uint8_t {
    FAULT_NONE,
    FAULT_BME_ROOM,
    FAULT_BME_OUTSIDE,
    FAULT_ADC_MQ,
    FAULT_SAFETY
};

enum VentReason : uint8_t {
    VENT_DAY_LOCK,
    VENT_NIGHT,
    VENT_BAD_AIR,
    VENT_CRITICAL_AIR,
    VENT_COOLING,
    VENT_OUTSIDE_HOT,
    VENT_OUTSIDE_COLD,
    VENT_OUTSIDE_WET,
    VENT_SENSOR_WAIT,
    VENT_FORCED,
    VENT_MANUAL_OFF,
    VENT_CYCLE_GUARD
};

enum ControllerState : uint8_t {
  CTRL_BOOT,
  CTRL_WARMUP,
  CTRL_NORMAL,
  CTRL_OFFLINE_LOGGING,
  CTRL_FAILSAFE,
  CTRL_MANUAL,
  CTRL_STANDBY
};

enum CalibrationState : uint8_t {
    CAL_IDLE,
    CAL_WARMUP,
    CAL_SAMPLING,
    CAL_DONE,
    CAL_FAILED
};

enum BootSequenceState : uint8_t {
    BOOT_IDLE,
    BOOT_CRT_POWER_ON,
    BOOT_CLEAR_SCREEN,
    BOOT_PRINT_HEADER,
    BOOT_PRINT_LINES,
    BOOT_PRINT_SEPARATOR,
    BOOT_PRINT_INIT_TEXT,
    BOOT_PRINT_INIT_DOTS,
    BOOT_PRINT_INIT_OK,
    BOOT_PRINT_DIAGNOSTICS,
    BOOT_PRINT_SEPARATOR2,
    BOOT_COMPLETE
};

enum EnhancedForecastMode : uint8_t {
    PRECIP_SOON,
    PRECIP_ATTENTION,
    PRECIP_WATCH,
    FRONT_PERSISTENT,
    CLEARING,
    STABLE,
    UNDEFINED
};

// --- Data Structures ---

struct VentDecision {
    bool relayOn = false;
    VentReason reason = VENT_SENSOR_WAIT;
};

struct PressureSample {
    time_t timestamp = 0;
    int pressureMm = 0;
};

struct PressureTrendResult {
    bool ready = false;
    float trendMmPerHour = 0.0f;
    uint8_t confidence = 0;
    uint8_t r2Score = 0;
    uint8_t countScore = 0;
    uint8_t spacingScore = 0;
    uint8_t sampleCount = 0;
    uint32_t spanSec = 0;
};

struct EnhancedForecast {
    EnhancedForecastMode mode = UNDEFINED;
    const char* tag = "[ НАКОПЛЕНИЕ ]";
    const char* trendName = "СТАБИЛЬНО";
    const char* windowText = "НЕ ОПРЕДЕЛЕНО";
    const char* line1 = "ИСТОРИЯ ДАВЛЕНИЯ";
    const char* line2 = "ЕЩЕ НАКАПЛИВАЕТСЯ";
    uint8_t probability = 0;
    uint8_t confidence = 0;
};

// --- Main Application State ---

struct AppState {
    // Core device state
    ControllerState controllerState = CTRL_BOOT;
    bool deviceAwake = true;
    bool bootComplete = false;
    uint32_t lastUpdateMs = 0;

    // Boot Sequence State Machine
    BootSequenceState bootSequenceState = BOOT_IDLE;
    uint32_t bootSequenceTimer = 0;
    int16_t bootSequenceHeight = 6;
    int16_t bootSequenceY = 12;
    int bootSequenceInitIndex = 0;
    int bootSequenceCycle = 0;
    int bootSequenceDotIndex = 0;

    // Sensor readings
    Readings current;
    Readings previous;

    // Sensor/Module status
    bool bmeRoomOnline = false;
    bool bmeOutsideOnline = false;
    bool mqSensorOnline = false;
    bool lightSensorOnline = false;
    bool relayModuleOnline = false;
    bool displayOnline = false;

    // Ventilation logic
    VentDecision ventDecision;
    bool manualVentMode = false;
    bool manualRelayOn = false;
    bool manualModeActive = false;  // Жесткий флаг ручного режима от тумблера
    unsigned long manualVentUntilMs = 0;
    bool relayOutputOn = false;
    unsigned long relayStateChangedMs = 0;
    bool relayGuardBypassOnce = false;

    // Display and UI
    uint8_t currentScreen = 0;
    bool displaySleeping = false;
    uint32_t frameCounter = 0;
    int currentHue = 0; // Текущее значение hue с потенциометра (0-255)

    // Themes
    bool amberTheme = false;
    bool diagnosticTheme = false;
    bool themeManualOverride = false;
    uint16_t activeThemeColor = THEME_GREEN;

    // Time synchronization
    bool timeSynced = false;
    bool timeSyncStarted = false;
    bool ntpConfigured = false;
    uint32_t lastTimeSyncAttemptMs = 0;
    uint32_t ntpConfiguredMs = 0;
    uint32_t timeSyncBackoffMs = TIME_SYNC_INITIAL_RETRY_MS;

    // PC Bridge
    bool pcBridgeConnected = false;
    bool pcBridgeAnimationArmed = false;
    bool pcBridgeAnimationPending = false;
    uint32_t pcBridgeAnimationArmedMs = 0;

    // Environmental state tracking
    bool nightMode = false;
    bool airQualityAlarm = false;
    bool airQualityCritical = false;
    bool coolingLatch = false;
    bool outsideAirDry = false;
    bool absoluteHumidityReady = false;
    bool outsideTempFallingFast = false;

    // Timers for state changes
    uint32_t darkSinceMs = 0;
    uint32_t lightSinceMs = 0;
    uint32_t hotSinceMs = 0;
    uint32_t coolSinceMs = 0;
    uint32_t badAirSinceMs = 0;
    uint32_t criticalAirSinceMs = 0;
    uint32_t goodAirSinceMs = 0;
    uint32_t dryAirSinceMs = 0;
    uint32_t wetAirSinceMs = 0;

    // MQ135 Gas Sensor State
    float mqAdcEma = 0.0f;
    bool mqAdcEmaReady = false;
    uint8_t mqAdcFaultCount = 0;
    bool mqReady = false;
    bool mqBurnInDone = false;
    bool mqBurnInLoaded = false;
    int mqBaselineAqi = 0;
    CalibrationState mqCalibrationState = CAL_IDLE;
    uint32_t mqCalibrationStartedMs = 0;
    uint32_t mqCalibrationSum = 0;
    uint32_t mqCalibrationCount = 0;
    uint32_t mqUntimedBurnInStartedMs = 0;
    time_t mqBurnInStartedAt = 0;

    // Pressure & Forecast
    PressureSample pressureHistory[PRESSURE_HISTORY_SIZE] = {};
    uint8_t pressureHistoryIndex = 0;
    uint8_t pressureSampleCount = 0;
    bool pressureHistoryDirty = false;
    uint32_t lastPressureSampleMs = 0;
    uint32_t lastPressureSaveMs = 0;
    int lastSavedPressureMm = -1;
    bool pressureTrendEmaReady = false;
    float pressureTrendEma = 0.0f;
    uint8_t pressureTrendConfidence = 0;
    time_t pressureTrendEmaSourceTs = 0;
    float forecastTrendMmPerHour = 0.0f;
    int forecastPressureMm = 0;

    // Faults & Safety
    FaultCode lastFaultCode = FAULT_NONE;
    uint8_t safetyFaultCode = FAULT_NONE;
    bool safetyFaultActive = false;
    uint32_t lastFaultSavedMs = 0;

    // Re-initialization timers
    uint32_t lastBmeRoomReinitMs = 0;
    uint32_t lastBmeOutsideReinitMs = 0;
};

// --- Global State and Mutex ---

extern AppState state;
extern SemaphoreHandle_t sharedStateMux;

#define LOCK_STATE() (sharedStateMux != NULL && xSemaphoreTake(sharedStateMux, portMAX_DELAY) == pdTRUE)
#define UNLOCK_STATE() xSemaphoreGive(sharedStateMux)

// --- Function Prototypes for State Management ---

void createSharedStateMux();
void copySharedReadings(Readings& readings, VentDecision& decision);
void publishSensorReadings(const Readings& nextReadings);
void publishVentDecision(const VentDecision& decision);

#endif // STATE_H
