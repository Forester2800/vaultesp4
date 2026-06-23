
#ifndef CONFIG_H
#define CONFIG_H

#include <TFT_eSPI.h>

// --- Базовая конфигурация ---
#define USE_MOCK_SENSORS false
#define USE_WIFI_TIME true
#define PERSIST_PRESSURE_HISTORY true

// --- Пины ---
#define BUTTON_PIN 27
#define RELAY_PIN 26
#define TOGGLE_MODE_PIN 16  // Тумблер №1: Режим Авто/Ручной (перенесено с GPIO 27 из-за конфликта)
#define TOGGLE_FAN_PIN 17   // Тумблер №2: Вентилятор Вкл/Выкл (перенесено с GPIO 14)
#define MODE_LED_PIN 2     // Светодиод-индикатор режима (зеленый) - перенесено на GPIO 2 (безопасный пин)
#define MODE_LED_PWM_FREQ  5000 // Высокая частота 5 кГц
#define MODE_LED_PWM_BITS  12   // 12-битное разрешение (0-4095)
#define BUZZER_PIN 14      // Пассивный пьезоизлучатель (бузер)
#define LIGHT_PIN 34 // Стандартный аналоговый режим (INPUT), делитель напряжения с внешним резистором
#define LDR_PIN LIGHT_PIN
#define PALETTE_PIN 33 // Потенциометр для управления палитрой цветов (INPUT)
#define SCREEN_SELECTOR_PIN 32 // Потенциометр для переключения экранов (INPUT)
#define MQ135_PIN 35 // GPIO 35 не поддерживает подтяжки, нужен внешний резистор
#define DISPLAY_BACKLIGHT_PIN 4 // Пин подсветки дисплея (GPIO 4 для ILI9341)
#define DISPLAY_BACKLIGHT_DEFAULT_PERCENT 70 // Яркость подсветки по умолчанию (уменьшено на 30% от 100%)
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// --- Дисплей ---
#define SCREEN_W 240
#define SCREEN_H 320
#define RELAY_ACTIVE_LOW false
#define DISPLAY_BACKLIGHT_ACTIVE_LOW false
#define DISPLAY_WAKE_DELAY_MS 140UL
#define DISPLAY_SLEEP_DELAY_MS 120UL
#define THEME_GREEN 0x07E8
#define THEME_AMBER 0xFDE0
#define THEME_DIAGNOSTIC 0x5DFF
#define BLACK TFT_BLACK
#define WARN TFT_RED

// --- I2C / BME280 ---
#define I2C_CLOCK_HZ 100000UL
#define I2C_TIMEOUT_MS 35UL
#define BME_ROOM_ADDR 0x76
#define BME_OUTSIDE_ADDR 0x77
#define BME_READ_RETRIES 2
#define BME_READ_RETRY_DELAY_MS 20UL
#define BME_REINIT_MS 30000UL

// --- Время / NTP ---
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC (3L * 60L * 60L)
#define DAYLIGHT_OFFSET_SEC 0
#define TIME_SYNC_INITIAL_RETRY_MS (60UL * 1000UL)
#define TIME_SYNC_MAX_RETRY_MS (30UL * 60UL * 1000UL)
#define TIME_SYNC_CONNECT_TIMEOUT_MS (15UL * 1000UL)
#define TIME_SYNC_NTP_TIMEOUT_MS (20UL * 1000UL)
#define MIN_VALID_EPOCH_TIME 1700000000UL
#define MIN_UPTIME_FOR_VALID_CLOCK_SEC 120UL

// --- Сенсоры / АЦП ---
#define ALTITUDE_METERS 145.0f
#define HPA_TO_MMHG 0.750061683f
#define ADC_EMA_ALPHA 0.10f
#define LIGHT_ADC_INVERT false

// --- MQ-135 ---
#define MQ_WARMUP_MS (3UL * 60UL * 1000UL)
#define MQ_CALIBRATION_MS (7UL * 60UL * 1000UL)
#define MQ_BAD_CONFIRM_MS (2UL * 60UL * 1000UL)
#define MQ_CRITICAL_CONFIRM_MS (10UL * 1000UL)
#define MQ_GOOD_CONFIRM_MS (5UL * 60UL * 1000UL)
#define MQ_FIRST_BURN_IN_REQUIRED true
#define MQ_ALLOW_UNTIMED_BURN_IN_FALLBACK true
#define MQ_FIRST_BURN_IN_SEC (24UL * 60UL * 60UL)
#define MQ_ADC_FAULT_CONFIRM_COUNT 3

// --- Кнопка / цикл UI ---
#define DEBOUNCE_MS 220UL
#define DOUBLE_CLICK_MS 380UL
#define POWER_HOLD_MS 4000UL
#define SENSOR_UPDATE_MS 400UL
#define BRIDGE_UI_TICK_MS 50UL

// --- Pressure / Forecast ---
#define PRESSURE_SAMPLE_MS (USE_MOCK_SENSORS ? 900UL : (5UL * 60UL * 1000UL))
#define PRESSURE_SAVE_MIN_INTERVAL_MS (USE_MOCK_SENSORS ? 10000UL : (15UL * 60UL * 1000UL))
#define PRESSURE_SAVE_PERIOD_MS (USE_MOCK_SENSORS ? 10000UL : (60UL * 60UL * 1000UL))
#define PRESSURE_SAVE_DELTA_MM 2
#define PRESSURE_HISTORY_SIZE (USE_MOCK_SENSORS ? 48 : 160)
#define PRESSURE_HISTORY_PURGE_THRESHOLD (PRESSURE_HISTORY_SIZE - 8)
#define PRESSURE_MAX_AGE_SEC (12UL * 60UL * 60UL)
#define PRESSURE_REGRESSION_MIN_SAMPLES 6
#define PRESSURE_TREND_SHORT_SEC (2UL * 60UL * 60UL)
#define PRESSURE_TREND_MEDIUM_SEC (5UL * 60UL * 60UL)
#define PRESSURE_TREND_LONG_SEC (10UL * 60UL * 60UL)
#define PRESSURE_TREND_SPAN_TOLERANCE_SEC (30UL * 60UL)
#define PRESSURE_TREND_EMA_ALPHA 0.35f
#define FORECAST_MIN_CONFIDENCE 25

// --- Безопасность / таймеры ---
#define WATCHDOG_TIMEOUT_SEC 30 // Увеличено для отладки boot loop
#define SAFETY_CHECK_MS 1000UL
#define FAULT_SAVE_MIN_INTERVAL_MS (60UL * 1000UL)

// --- Serial Bridge ---
#define PC_BRIDGE_ID "soch-tec-pc-bridge"
#define PC_BRIDGE_PROTOCOL_VERSION 2
#define PC_BRIDGE_TIMEOUT_MS 15000UL
#define PC_BRIDGE_BOOT_ANIMATION_DELAY_MS 900UL
#define SERIAL_STREAM_MS 1000UL
#define SERIAL_HISTORY_BATCH_SIZE 6
#define SERIAL_HISTORY_BATCH_INTERVAL_MS 20UL

// --- Bring-up diagnostics ---
#define BRINGUP_SERIAL_DIAGNOSTICS true
#define BRINGUP_WIFI_DIAGNOSTICS true
#define BRINGUP_HEARTBEAT_MS 3000UL
#define BRINGUP_WIFI_TIMEOUT_MS 20000UL
#define BRINGUP_NTP_TIMEOUT_MS 15000UL
#define BRINGUP_FORCE_MOCK_SENSORS false

// --- Значения по умолчанию для module_settings ---
#define ROOM_HOT_ON 22.0f
#define ROOM_COOL_OFF 19.0f
#define OUTSIDE_COMFORT_MIN 10.0f
#define OUTSIDE_COMFORT_MAX 24.0f
#define OUTSIDE_SAFE_MIN 5.0f
#define OUTSIDE_SAFE_MAX 28.0f
#define OUTSIDE_EMERGENCY_MIN -10.0f
#define OUTSIDE_EMERGENCY_MAX 35.0f
#define COOLING_DELTA_C 1.5f
#define OUTSIDE_HUMIDITY_LOCK 82.0f
#define ABS_HUMIDITY_DRY_ON_DELTA 0.8f
#define ABS_HUMIDITY_DRY_OFF_DELTA 0.2f
#define PREDICTIVE_ROOM_HOLD_C 21.0f
#define MQ_BAD_ON 170
#define MQ_GOOD_OFF 130
#define MQ_CRITICAL_ON 350
#define LIGHT_NIGHT_ON 35
#define LIGHT_DAY_OFF 60
#define LIGHT_CONFIRM_MS (5UL * 60UL * 1000UL)
#define THERMAL_HOT_CONFIRM_MS (3UL * 60UL * 1000UL)
#define THERMAL_COOL_CONFIRM_MS (5UL * 60UL * 1000UL)
#define RELAY_MIN_SWITCH_MS (2UL * 60UL * 1000UL)

#endif // CONFIG_H
