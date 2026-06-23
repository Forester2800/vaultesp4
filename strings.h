#ifndef STRINGS_H
#define STRINGS_H

#pragma once

// User-facing strings for the device UI.
// static gives internal linkage and avoids multiple-definition linker errors
// when this header is included in several translation units.

// --- Time & Date ---
static const char STR_TIME_FORMAT[] = "%H:%M";
static const char STR_DATE_FORMAT[] = "%d.%m.%y";
static const char STR_DATE_UNKNOWN[] = "--.--.--";

// --- Main Screen ---
static const char STR_UNIT_MMHG[] = "мм";
static const char STR_PC_STATUS[] = "PC";
static const char STR_SECTION_INDOOR[] = "> ЖИЛОЙ БЛОК";
static const char STR_SECTION_OUTDOOR[] = "> ВНЕШНИЙ ПЕРИМЕТР";
static const char STR_SECTION_VENT[] = "КОНТУР ПРИТОКА";
static const char STR_LABEL_TEMP[] = "ТЕМП:";
static const char STR_LABEL_HUM[] = "ВЛАЖ:";
static const char STR_LABEL_PRESSURE[] = "ДАВЛ:";
static const char STR_LABEL_GAS[] = "ГАЗ :";
static const char STR_LABEL_LIGHT[] = "СВЕТ:";

// --- Gas Sensor States ---
static const char STR_GAS_STATE_OK[] = "НОРМА";
static const char STR_GAS_STATE_BAD[] = "ПЛОХО";
static const char STR_GAS_STATE_CRITICAL[] = "КРИТИЧ.";
static const char STR_GAS_STATE_CALIBRATING[] = "КАЛИБР.";
static const char STR_GAS_STATE_WARMING_UP[] = "ПРОГРЕВ";
static const char STR_GAS_STATE_FAULT[] = "НЕИСПР.";

// --- Light Modes ---
static const char STR_LIGHT_MODE_NIGHT[] = "НОЧЬ";
static const char STR_LIGHT_MODE_DAY[] = "ДЕНЬ";

// --- Vent Reasons ---
static const char STR_VENT_REASON_NIGHT_PROTOCOL[] = "НОЧНОЙ ПРОТОКОЛ";
static const char STR_VENT_REASON_CLEAN_ATMOSPHERE[] = "ОЧИСТКА АТМОСФЕРЫ";
static const char STR_VENT_REASON_EMERGENCY_PROTOCOL[] = "АВАРИЙНЫЙ ПРОТОКОЛ";
static const char STR_VENT_REASON_COOLING[] = "ОХЛАЖДЕНИЕ";
static const char STR_VENT_REASON_OUTSIDE_HOT[] = "СНАРУЖИ ЖАРКО";
static const char STR_VENT_REASON_OUTSIDE_COLD[] = "СНАРУЖИ ХОЛОДНО";
static const char STR_VENT_REASON_OUTSIDE_WET[] = "СНАРУЖИ ВЛАЖНО";
static const char STR_VENT_REASON_SENSOR_WAIT[] = "КАЛИБРОВКА";
static const char STR_VENT_REASON_FORCED[] = "ДОПУСК СМОТРИТЕЛЯ";
static const char STR_VENT_REASON_MANUAL_OFF[] = "СМОТРИТЕЛЬ: ВЫКЛ";
static const char STR_VENT_REASON_CYCLE_GUARD[] = "ЗАЩИТА РЕЛЕ";
static const char STR_VENT_REASON_DAY_LOCK[] = "ДНЕВНОЙ ПРОТОКОЛ";

// --- Pressure Trends ---
static const char STR_TREND_RISING[] = "РОСТ";
static const char STR_TREND_FALLING[] = "ПАДЕНИЕ";
static const char STR_TREND_STABLE[] = "СТАБИЛЬНО";

// --- PC Bridge Animation ---
static const char STR_PC_BRIDGE_TITLE[] = ">>> ТЕРМИНАЛЬНЫЙ МОСТ <<<";
static const char STR_PC_BRIDGE_DONT_TURN_OFF[] = "НЕ ВЫКЛЮЧАЙТЕ УСТРОЙСТВО";
static const char STR_PC_BRIDGE_CONNECTED[] = "ПОДКЛЮЧЕНО К PC";
static const char STR_PC_BRIDGE_DIRECT_CONTROL[] = "РЕЖИМ ПРЯМОГО УПРАВЛЕНИЯ";
static const char STR_PC_BRIDGE_WAITING[] = "ОЖИДАНИЕ КОМАНД...";

// --- Boot Sequence ---
static const char STR_BOOT_TITLE[] = "S.O.C.H.-VCS Mk II";
static const char STR_BOOT_LOADING[] = "ЗАГРУЗКА...";
static const char STR_BOOT_OK_CORE[] = "[✓] ЯДРО СИСТЕМЫ";
static const char STR_BOOT_OK_BME[] = "[✓] СЕНСОРЫ BME280";
static const char STR_BOOT_OK_MQ[] = "[✓] СЕНСОР MQ-135";
static const char STR_BOOT_WAIT_TIME[] = "[ ] СИНХР. ВРЕМЕНИ";
static const char STR_BOOT_OK_TIME[] = "[✓] СИНХР. ВРЕМЕНИ";

// --- Forecast Screen ---
static const char STR_FORECAST_PRESSURE[] = "%3d мм рт. ст.";
static const char STR_FORECAST_TREND[] = "%s [%+d мм/ч]";
static const char STR_FORECAST_CONFIDENCE[] = "%3u%%";
static const char STR_FORECAST_WINDOW[] = "ОКНО: %s";
static const char STR_FORECAST_PROBABILITY[] = "ВЕРОЯТН:%3u%% ДОСТ:%3u%%";
static const char STR_FORECAST_TITLE[] = "> S.O.C.H.-Tec МЕТЕОПОСТ";
static const char STR_FORECAST_SUBTITLE[] = "  ПРОГНОЗ ПЕРИМЕТРА";
static const char STR_FORECAST_SEPARATOR[] = "========================";
static const char STR_LABEL_CURRENT_P[] = "* P_ТЕКУЩ:";
static const char STR_LABEL_DYNAMICS[] = "ДИНАМИКА:";
static const char STR_LABEL_CONFIDENCE[] = "ДОСТОВЕРН.:";
static const char STR_LABEL_ANALYSIS[] = "> АНАЛИЗ ФРОНТА...";
static const char STR_ANALYSIS_DONE[] = "[ ВЫПОЛНЕН ]";
static const char STR_ANALYSIS_ACCUM[] = "[ НАКОПЛ. ]";
static const char STR_LABEL_FORECAST_WINDOW[] = "> ОКНО ПРОГНОЗА 12 Ч:";

// --- Forecast Statuses ---
static const char STR_FORECAST_STATUS_WAITING[] = "[ ОЖИДАНИЕ ]";
static const char STR_FORECAST_STATUS_NTP_SYNC[] = "СИНХРОНИЗАЦИЯ NTP";
static const char STR_FORECAST_STATUS_HISTORY_INACTIVE[] = "ИСТОРИЯ НЕ АКТИВНА";
static const char STR_FORECAST_STATUS_ACCUMULATING[] = "[ НАКОПЛЕНИЕ ]";
static const char STR_FORECAST_STATUS_PRESSURE_HISTORY[] = "ИСТОРИЯ ДАВЛЕНИЯ";
static const char STR_FORECAST_STATUS_FORECAST_PENDING[] = "ПРОГНОЗ ОЖИДАЕТ";
static const char STR_FORECAST_UNDEFINED[] = "НЕ ОПРЕДЕЛЕНО";
static const char STR_FORECAST_ACCUMULATING_DETAILS[] = "ЕЩЕ НАКАПЛИВАЕТСЯ";

// --- Forecast Tags ---
static const char STR_FORECAST_TAG_SOON[] = "[ СКОРО ]";
static const char STR_FORECAST_TAG_ATTENTION[] = "[ ВНИМАНИЕ ]";
static const char STR_FORECAST_TAG_FRONT[] = "[ ФРОНТ ]";
static const char STR_FORECAST_TAG_WATCH[] = "[ НАБЛЮДЕНИЕ ]";
static const char STR_FORECAST_TAG_CLEARING[] = "[ ПРОЯСНЕНИЕ ]";
static const char STR_FORECAST_TAG_STABLE[] = "[ НОРМА ]";

// --- Forecast Windows ---
static const char STR_FORECAST_WINDOW_1_3H[] = "1-3 Ч";
static const char STR_FORECAST_WINDOW_3_6H[] = "3-6 Ч";
static const char STR_FORECAST_WINDOW_6_12H[] = "6-12 Ч";
static const char STR_FORECAST_WINDOW_2_6H[] = "2-6 Ч";
static const char STR_FORECAST_WINDOW_12H[] = "12 Ч";

// --- Forecast Lines ---
static const char STR_FORECAST_LINE1_PRECIP_PROBABLE[] = "ОСАДКИ ВЕРОЯТНЫ";
static const char STR_FORECAST_LINE2_FRONT_APPROACHING[] = "ФРОНТ ПРИБЛИЖАЕТСЯ";
static const char STR_FORECAST_LINE1_PRECIP_POSSIBLE[] = "ВОЗМОЖНЫ ОСАДКИ";
static const char STR_FORECAST_LINE2_PRESSURE_FALLING[] = "ДАВЛЕНИЕ ПАДАЕТ";
static const char STR_FORECAST_LINE1_BAD_WEATHER_MAY[] = "НЕПОГОДА МОЖЕТ";
static const char STR_FORECAST_LINE2_PERSIST[] = "СОХРАНЯТЬСЯ";
static const char STR_FORECAST_LINE1_PRECIP_RISK[] = "РИСК ОСАДКОВ";
static const char STR_FORECAST_LINE2_NEEDS_MONITORING[] = "ТРЕБУЕТ КОНТРОЛЯ";
static const char STR_FORECAST_LINE1_CONDITIONS_IMPROVING[] = "УСЛОВИЯ УЛУЧШАЮТСЯ";
static const char STR_FORECAST_LINE2_FRONT_WEAKENING[] = "ФРОНТ ОСЛАБЕВАЕТ";
static const char STR_FORECAST_LINE1_NO_SIGNIFICANT[] = "СУЩЕСТВЕННЫХ";
static const char STR_FORECAST_LINE2_CHANGES[] = "ИЗМЕНЕНИЙ НЕТ";

// --- System Identifiers ---
static const char STR_DEVICE_ID[] = "soch-tec-esp32-vault-panel";
static const char STR_FIRMWARE_VERSION[] = "0.3.0-audit-refactor";
static const char STR_PC_BRIDGE_ID[] = "soch-tec-pc-bridge";

// --- Persistence Namespaces ---
static const char STR_PRESSURE_PREFS_NAMESPACE[] = "pressure";
static const char STR_MQ_PREFS_NAMESPACE[] = "mq135";
static const char STR_SAMPLE_PREFS_NAMESPACE[] = "samples";
static const char STR_FAULT_PREFS_NAMESPACE[] = "faults";

#endif // STRINGS_H
