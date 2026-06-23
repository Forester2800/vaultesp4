#include "module_serial_bridge.h"

#include "config.h"
#include "module_ui_helpers.h"
#include "state.h"
#include "strings.h"
#include "module_display.h"
#include "module_relay_io.h"

#include <Arduino.h>
#include <time.h>
#include <string.h>

// Реализация находится в `module_time_sync.h` (подключён в основном `.ino`).
bool hasValidClock();

namespace {
constexpr size_t SERIAL_RX_BUFFER_SIZE = 256;
constexpr size_t BRIDGE_HISTORY_SIZE = 180;
constexpr size_t HISTORY_CHUNK_SIZE = 8;

struct BridgeSample {
    uint32_t ts;
    uint32_t deviceTs;
    bool clockValid;
    uint32_t uptime;
    float roomTemp;
    float outTemp;
    float roomHum;
    float outHum;
    int pressure;
    int gas;
    int light;
    bool relay;
    bool awake;
    uint8_t screen;
    bool manualMode;
    bool manualRelay;
    bool manualModeActive;
    bool pcBridge;
    char theme[16];
    char reason[24];
};

BridgeSample historyBuffer[BRIDGE_HISTORY_SIZE] = {};
size_t historyWriteIndex = 0;
size_t historyCount = 0;

char serialRxBuffer[SERIAL_RX_BUFFER_SIZE] = {};
size_t serialRxLength = 0;
bool serialRxOverflow = false;

bool historyDumpActive = false;
uint32_t historyDumpSince = 0;
size_t historyDumpCursor = 0;
uint32_t lastSerialStreamMs = 0;
uint32_t lastCaptureMs = 0;
uint32_t lastPcBridgeMs = 0;
bool pcStreamEnabled = false;
uint32_t serialCommandSeq = 0;
char pcBridgeSession[32] = "";

uint32_t deviceUptimeSec() {
    return millis() / 1000UL;
}

const char* activeThemeName() {
    if (state.diagnosticTheme) return "diagnostic";
    return state.amberTheme ? "amber" : "green";
}

const char* currentReasonCode() {
    if (state.manualVentMode) {
        return state.manualRelayOn ? "FORCED" : "MANUAL_OFF";
    }
    return "DAY_LOCK";
}

bool jsonHasCommand(const char* line, const char* name) {
    const char* key = "\"name\":\"";
    const char* p = strstr(line, key);
    if (!p) return false;
    p += strlen(key);
    return strncmp(p, name, strlen(name)) == 0;
}

bool jsonExtractString(const char* line, const char* field, char* out, size_t outSize) {
    char needle[48];
    snprintf(needle, sizeof(needle), "\"%s\":\"", field);
    const char* start = strstr(line, needle);
    if (!start) return false;
    start += strlen(needle);
    const char* end = strchr(start, '"');
    if (!end) return false;
    size_t len = static_cast<size_t>(end - start);
    if (len >= outSize) len = outSize - 1;
    memcpy(out, start, len);
    out[len] = '\0';
    return true;
}

uint32_t jsonExtractUInt(const char* line, const char* field, uint32_t fallback = 0) {
    char needle[48];
    snprintf(needle, sizeof(needle), "\"%s\":", field);
    const char* p = strstr(line, needle);
    if (!p) return fallback;
    p += strlen(needle);
    return static_cast<uint32_t>(strtoul(p, nullptr, 10));
}

bool jsonExtractBool(const char* line, const char* field, bool fallback) {
    char needle[48];
    snprintf(needle, sizeof(needle), "\"%s\":", field);
    const char* p = strstr(line, needle);
    if (!p) return fallback;
    p += strlen(needle);
    if (strncmp(p, "true", 4) == 0) return true;
    if (strncmp(p, "false", 5) == 0) return false;
    return fallback;
}

void rememberCommandMeta(const char* line) {
    serialCommandSeq = jsonExtractUInt(line, "seq", 0);
    jsonExtractString(line, "session", pcBridgeSession, sizeof(pcBridgeSession));
    lastPcBridgeMs = millis();
    state.pcBridgeConnected = true;
}

void printJsonString(const char* key, const char* value) {
    Serial.print("\"");
    Serial.print(key);
    Serial.print("\":\"");
    for (const char* p = value; *p; ++p) {
        if (*p == '"' || *p == '\\') {
            Serial.print('\\');
        }
        Serial.print(*p);
    }
    Serial.print("\"");
}

void writeSampleJson(const char* type, const BridgeSample& s) {
    Serial.print("{");
    printJsonString("type", type);
    Serial.printf(",\"ts\":%lu", static_cast<unsigned long>(s.ts));
    Serial.printf(",\"deviceTs\":%lu", static_cast<unsigned long>(s.deviceTs));
    Serial.printf(",\"clockValid\":%s", s.clockValid ? "true" : "false");
    Serial.printf(",\"uptime\":%lu", static_cast<unsigned long>(s.uptime));
    Serial.printf(",\"roomTemp\":%.1f", s.roomTemp);
    Serial.printf(",\"outTemp\":%.1f", s.outTemp);
    Serial.printf(",\"roomHum\":%.0f", s.roomHum);
    Serial.printf(",\"outHum\":%.0f", s.outHum);
    Serial.printf(",\"pressure\":%d", s.pressure);
    Serial.printf(",\"gas\":%d", s.gas);
    Serial.printf(",\"light\":%d", s.light);
    Serial.printf(",\"relay\":%s", s.relay ? "true" : "false");
    Serial.printf(",\"awake\":%s", s.awake ? "true" : "false");
    Serial.printf(",\"screen\":%u", s.screen);
    Serial.printf(",\"manualMode\":%s", s.manualMode ? "true" : "false");
    Serial.printf(",\"manualRelay\":%s", s.manualRelay ? "true" : "false");
    Serial.printf(",\"manualModeActive\":%s", s.manualModeActive ? "true" : "false");
    Serial.printf(",\"pcBridge\":%s", s.pcBridge ? "true" : "false");
    Serial.print(",");
    printJsonString("theme", s.theme);
    Serial.print(",");
    printJsonString("reason", s.reason);
    Serial.println("}");
}

BridgeSample makeCurrentSample() {
    BridgeSample s{};
    const bool clockValid = hasValidClock();
    const uint32_t uptime = deviceUptimeSec();
    const uint32_t ts = clockValid ? static_cast<uint32_t>(time(nullptr)) : uptime;
    s.ts = ts;
    s.deviceTs = ts;
    s.clockValid = clockValid;
    s.uptime = uptime;
    s.roomTemp = state.current.tempRoom;
    s.outTemp = state.current.tempOutside;
    s.roomHum = state.current.humRoom;
    s.outHum = state.current.humOutside;
    s.pressure = state.current.pressureOutsideMm > 0 ? state.current.pressureOutsideMm : state.current.pressureRoomMm;
    s.gas = state.current.co2;
    s.light = state.current.lightPercent;
    s.relay = state.manualRelayOn || state.relayOutputOn;
    s.awake = !state.displaySleeping;
    s.screen = state.currentScreen;
    s.manualMode = state.manualVentMode;
    s.manualRelay = state.manualRelayOn;
    s.manualModeActive = state.manualModeActive;
    s.pcBridge = state.pcBridgeConnected;
    strncpy(s.theme, activeThemeName(), sizeof(s.theme) - 1);
    strncpy(s.reason, currentReasonCode(), sizeof(s.reason) - 1);
    return s;
}

void storeHistorySample(const BridgeSample& sample) {
    historyBuffer[historyWriteIndex] = sample;
    historyWriteIndex = (historyWriteIndex + 1) % BRIDGE_HISTORY_SIZE;
    if (historyCount < BRIDGE_HISTORY_SIZE) {
        historyCount++;
    }
}

BridgeSample historySampleAt(size_t logicalIndex) {
    const size_t start = (historyWriteIndex + BRIDGE_HISTORY_SIZE - historyCount) % BRIDGE_HISTORY_SIZE;
    const size_t realIndex = (start + logicalIndex) % BRIDGE_HISTORY_SIZE;
    return historyBuffer[realIndex];
}

void captureSampleIfDue() {
    const uint32_t now = millis();
    if (now - lastCaptureMs < SERIAL_STREAM_MS) {
        return;
    }
    lastCaptureMs = now;
    storeHistorySample(makeCurrentSample());
}

void printCommandAck(const char* name) {
    Serial.print("{\"type\":\"ack\",");
    printJsonString("name", name);
    if (serialCommandSeq > 0) {
        Serial.printf(",\"seq\":%lu", static_cast<unsigned long>(serialCommandSeq));
    }
    if (pcBridgeSession[0] != '\0') {
        Serial.print(",");
        printJsonString("session", pcBridgeSession);
    }
    Serial.printf(",\"clockValid\":%s,\"uptime\":%lu}\n",
                  hasValidClock() ? "true" : "false",
                  static_cast<unsigned long>(deviceUptimeSec()));
}

void printControlAck(const char* name, bool value) {
    Serial.print("{\"type\":\"ack\",");
    printJsonString("name", name);
    Serial.printf(",\"value\":%s", value ? "true" : "false");
    Serial.printf(",\"manualMode\":%s", state.manualVentMode ? "true" : "false");
    Serial.printf(",\"manualRelay\":%s", state.manualRelayOn ? "true" : "false");
    if (serialCommandSeq > 0) {
        Serial.printf(",\"seq\":%lu", static_cast<unsigned long>(serialCommandSeq));
    }
    Serial.printf(",\"clockValid\":%s,\"uptime\":%lu}\n",
                  hasValidClock() ? "true" : "false",
                  static_cast<unsigned long>(deviceUptimeSec()));
    vTaskDelay(5 / portTICK_PERIOD_MS); // Задержка для завершения отправки данных после потенциальных помех
}

void handleThemeCommand(const char* line) {
    char value[16] = "";
    if (!jsonExtractString(line, "value", value, sizeof(value))) {
        jsonExtractString(line, "theme", value, sizeof(value));
    }

    if (strcmp(value, "amber") == 0) {
        state.amberTheme = true;
        state.diagnosticTheme = false;
    } else if (strcmp(value, "diagnostic") == 0) {
        state.amberTheme = false;
        state.diagnosticTheme = true;
    } else {
        state.amberTheme = false;
        state.diagnosticTheme = false;
    }
    applyThemeColor();
    printCommandAck("theme");
}
}

void printHelloAck() {
    Serial.print("{\"type\":\"ack\",");
    printJsonString("name", "hello");
    Serial.print(",");
    printJsonString("deviceId", STR_DEVICE_ID);
    Serial.print(",");
    printJsonString("firmware", STR_FIRMWARE_VERSION);
    Serial.printf(",\"protocol\":%d", PC_BRIDGE_PROTOCOL_VERSION);
    Serial.printf(",\"sampleMs\":%d", SERIAL_STREAM_MS);
    Serial.printf(",\"timeoutMs\":%d", PC_BRIDGE_TIMEOUT_MS);
    if (serialCommandSeq > 0) {
        Serial.printf(",\"seq\":%lu", static_cast<unsigned long>(serialCommandSeq));
    }
    if (pcBridgeSession[0] != '\0') {
        Serial.print(",");
        printJsonString("session", pcBridgeSession);
    }
    Serial.printf(",\"clockValid\":%s,\"uptime\":%lu",
                  hasValidClock() ? "true" : "false",
                  static_cast<unsigned long>(deviceUptimeSec()));
    Serial.print(",\"capabilities\":[\"samples\",\"history\",\"manual_relay\",\"screen\",\"theme\",\"standby\",\"server_session\",\"stream_control\",\"device_status\"]");
    Serial.println("}");
}

void printDeviceStatus() {
    Serial.print("{\"type\":\"device_status\"");
    if (serialCommandSeq > 0) {
        Serial.printf(",\"seq\":%lu", static_cast<unsigned long>(serialCommandSeq));
    }
    if (pcBridgeSession[0] != '\0') {
        Serial.print(",");
        printJsonString("session", pcBridgeSession);
    }
    Serial.print(",");
    printJsonString("deviceId", STR_DEVICE_ID);
    Serial.print(",");
    printJsonString("firmware", STR_FIRMWARE_VERSION);
    Serial.printf(",\"protocol\":%d", PC_BRIDGE_PROTOCOL_VERSION);
    Serial.print(",\"state\":\"bringup\"");
    Serial.printf(",\"freeHeap\":%u,\"minFreeHeap\":%u", ESP.getFreeHeap(), ESP.getMinFreeHeap());
    Serial.printf(",\"clockValid\":%s,\"uptime\":%lu", hasValidClock() ? "true" : "false", static_cast<unsigned long>(deviceUptimeSec()));
    Serial.printf(",\"pcBridge\":%s,\"stream\":%s", state.pcBridgeConnected ? "true" : "false", pcStreamEnabled ? "true" : "false");
    Serial.printf(",\"sampleRamCount\":%u,\"sampleRamSize\":%u", static_cast<unsigned>(historyCount), static_cast<unsigned>(BRIDGE_HISTORY_SIZE));
    Serial.print(",\"sampleFsReady\":false,\"sampleFsCount\":0,\"sampleFsSize\":0");
    Serial.printf(",\"manualMode\":%s,\"manualRelay\":%s", state.manualVentMode ? "true" : "false", state.manualRelayOn ? "true" : "false");
    Serial.printf(",\"relay\":%s", (state.manualRelayOn || state.relayOutputOn) ? "true" : "false");
    Serial.printf(",\"bmeRoom\":%s,\"bmeOutside\":%s", state.bmeRoomOnline ? "true" : "false", state.bmeOutsideOnline ? "true" : "false");
    Serial.printf(",\"mqOnline\":%s,\"mqReady\":%s,\"mqBurnInDone\":%s",
                  state.mqSensorOnline ? "true" : "false",
                  state.mqReady ? "true" : "false",
                  state.mqBurnInDone ? "true" : "false");
    Serial.printf(",\"mqBaselineAqi\":%lu", static_cast<unsigned long>(state.mqBaselineAqi));
    Serial.printf(",\"lightSensor\":%s,\"relayModule\":true", state.current.lightPercent >= 0 ? "true" : "false");
    Serial.println("}");
}

void handleSerialCommand(const char* line) {
    const bool isHello = jsonHasCommand(line, "hello");
    const bool isHeartbeat = jsonHasCommand(line, "heartbeat");
    const bool isServerOnline = jsonHasCommand(line, "server_online");
    const bool isStreamStart = jsonHasCommand(line, "stream_start");
    const bool isStreamStop = jsonHasCommand(line, "stream_stop");
    const bool isDeviceStatus = jsonHasCommand(line, "device_status");
    const bool isDumpSince = jsonHasCommand(line, "dump_since");
    const bool isHistoryAck = jsonHasCommand(line, "history_ack");
    const bool isManualMode = jsonHasCommand(line, "manual_mode");
    const bool isManualRelay = jsonHasCommand(line, "manual_relay") || jsonHasCommand(line, "force_relay");
    const bool isStandby = jsonHasCommand(line, "standby");
    const bool isScreen = jsonHasCommand(line, "screen");
    const bool isTheme = jsonHasCommand(line, "theme");
    const bool isSyncResponse = jsonHasCommand(line, "sync_response");

    if (!(isHello || isHeartbeat || isServerOnline || isStreamStart || isStreamStop || isDeviceStatus ||
          isDumpSince || isHistoryAck || isManualMode || isManualRelay || isStandby || isScreen || isTheme || isSyncResponse)) {
        return;
    }

    rememberCommandMeta(line);

    if (isHello) {
        printHelloAck();
        return;
    }
    if (isHeartbeat) {
        printCommandAck("heartbeat");
        return;
    }
    if (isServerOnline) {
        pcStreamEnabled = jsonExtractBool(line, "wantStream", pcStreamEnabled);
        printCommandAck("server_online");
        return;
    }
    if (isStreamStart) {
        pcStreamEnabled = true;
        lastSerialStreamMs = 0;
        printCommandAck("stream_start");
        return;
    }
    if (isStreamStop) {
        pcStreamEnabled = false;
        printCommandAck("stream_stop");
        return;
    }
    if (isDeviceStatus) {
        printDeviceStatus();
        return;
    }
    if (isDumpSince) {
        historyDumpSince = jsonExtractUInt(line, "ts", 0);
        historyDumpCursor = 0;
        historyDumpActive = true;
        printCommandAck("dump_since");
        return;
    }
    if (isHistoryAck) {
        const uint32_t upTo = jsonExtractUInt(line, "upTo", 0);
        while (historyCount > 0) {
            BridgeSample oldest = historySampleAt(0);
            const uint32_t marker = oldest.clockValid ? oldest.ts : oldest.deviceTs;
            if (marker > upTo) {
                break;
            }
            const size_t start = (historyWriteIndex + BRIDGE_HISTORY_SIZE - historyCount) % BRIDGE_HISTORY_SIZE;
            (void)start;
            historyCount--;
        }
        return;
    }
    if (isManualMode) {
        // Если тумблер режима замкнут (ручной режим), игнорируем команды от сервера
        if (state.manualModeActive) {
            printControlAck("manual_mode", state.manualVentMode);
            return;
        }
        state.manualVentMode = jsonExtractBool(line, "value", state.manualVentMode);
        if (!state.manualVentMode) {
            state.manualRelayOn = false;
            state.manualVentUntilMs = 0;
        }
        printControlAck("manual_mode", state.manualVentMode);
        return;
    }
    if (isManualRelay) {
        // Если тумблер режима замкнут (ручной режим), игнорируем команды от сервера
        if (state.manualModeActive) {
            printControlAck("manual_relay", state.manualRelayOn);
            return;
        }
        const bool value = jsonExtractBool(line, "value", state.manualRelayOn);
        const uint32_t ttlSec = jsonExtractUInt(line, "ttlSec", 0);
        state.manualVentMode = value ? true : state.manualVentMode;
        state.manualRelayOn = value;
        state.manualVentUntilMs = (value && ttlSec > 0) ? (millis() + ttlSec * 1000UL) : 0;
        printControlAck("manual_relay", state.manualRelayOn);
        return;
    }
    if (isStandby) {
        const bool standby = jsonExtractBool(line, "value", state.displaySleeping);
        if (standby) {
            displaySleep();
        } else {
            displayWake();
        }
        printControlAck("standby", standby);
        return;
    }
    if (isScreen) {
        state.currentScreen = static_cast<uint8_t>(jsonExtractUInt(line, "value", state.currentScreen)) % 3;
        printCommandAck("screen");
        return;
    }
    if (isTheme) {
        handleThemeCommand(line);
        return;
    }
    if (isSyncResponse) {
        // Если тумблер режима замкнут (ручной режим), игнорируем sync_response для manualMode и manualRelay
        if (!state.manualModeActive) {
            state.manualModeActive = jsonExtractBool(line, "manualModeActive", state.manualModeActive);
            state.manualVentMode = jsonExtractBool(line, "manualMode", state.manualVentMode);
            state.manualRelayOn = jsonExtractBool(line, "manualRelay", state.manualRelayOn);
            // Применяем синхронизированное состояние реле
            setRelayOutput(state.manualRelayOn);
        }
        printControlAck("sync_response", state.manualRelayOn);
        return;
    }
}

void touchPcBridge() {
    const bool wasConnected = state.pcBridgeConnected;
    state.pcBridgeConnected = true;
    lastPcBridgeMs = millis();
    if (!wasConnected && state.deviceAwake && state.bootComplete) {
        showPcBridgeAnimation();
        // Запрашиваем синхронизацию состояния с сервером при установлении соединения
        Serial.print("{\"type\":\"sync_request\"}\n");
    }
}

void streamCurrentSample() {
    const uint32_t now = millis();
    if (!pcStreamEnabled || (now - lastSerialStreamMs < SERIAL_STREAM_MS)) {
        return;
    }
    lastSerialStreamMs = now;
    BridgeSample sample = makeCurrentSample();
    writeSampleJson("sample", sample);
}

void pumpHistoryDump() {
    if (!historyDumpActive) {
        return;
    }

    size_t sent = 0;
    while (historyDumpCursor < historyCount && sent < HISTORY_CHUNK_SIZE) {
        BridgeSample sample = historySampleAt(historyDumpCursor++);
        const uint32_t marker = sample.clockValid ? sample.ts : sample.deviceTs;
        if (marker <= historyDumpSince) {
            continue;
        }
        writeSampleJson("history", sample);
        sent++;
    }

    if (historyDumpCursor >= historyCount) {
        Serial.printf("{\"type\":\"dump_done\",\"upTo\":%lu,\"count\":%u,\"source\":\"ram\"}\n",
                      static_cast<unsigned long>(historyCount > 0 ? historySampleAt(historyCount - 1).ts : 0),
                      static_cast<unsigned>(historyCount));
        historyDumpActive = false;
        historyDumpCursor = 0;
    }
}

void maintainPcBridgeState() {
    captureSampleIfDue();

    // Периодическая отправка hello для инициализации соединения (каждые 5 секунд если нет соединения)
    static unsigned long lastHelloMs = 0;
    if (!state.pcBridgeConnected && (millis() - lastHelloMs > 5000)) {
        lastHelloMs = millis();
        Serial.print("{\"type\":\"hello\",\"deviceId\":\"");
        Serial.print(STR_DEVICE_ID);
        Serial.println("\"}");
    }

    while (Serial.available() > 0) {
        const char ch = static_cast<char>(Serial.read());
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            serialRxBuffer[serialRxLength] = '\0';
            if (!serialRxOverflow && serialRxLength > 0) {
                handleSerialCommand(serialRxBuffer);
            }
            serialRxLength = 0;
            serialRxOverflow = false;
            continue;
        }
        if (serialRxLength + 1 < SERIAL_RX_BUFFER_SIZE) {
            serialRxBuffer[serialRxLength++] = ch;
        } else {
            serialRxOverflow = true;
        }
    }

    if (state.pcBridgeConnected && millis() - lastPcBridgeMs > PC_BRIDGE_TIMEOUT_MS) {
        state.pcBridgeConnected = false;
        pcStreamEnabled = false;
        pcBridgeSession[0] = '\0';
        serialCommandSeq = 0;
    }

    streamCurrentSample();
}
