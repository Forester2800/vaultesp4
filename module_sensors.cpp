#include "module_sensors.h"
#include "state.h"
#include "config.h"
#include <Wire.h>
#include <math.h>

namespace {
    // Вспомогательная функция для преобразования давления в мм рт. ст.
    int bmePressureToSeaLevelMm(float pressureHpa, Adafruit_BME280& bme) {
        float seaLevelHpa = bme.seaLevelForAltitude(ALTITUDE_METERS, pressureHpa);
        return (int)(seaLevelHpa * 0.750062f);
    }

    // Анонимное пространство имен для внутренних функций модуля
    bool reinitializeBmeIfDue(Adafruit_BME280& bme, uint8_t addr, unsigned long& lastReinitMs, bool& onlineStatus) {
        unsigned long now = millis();
        if (!onlineStatus && (now - lastReinitMs > BME_REINIT_MS)) {
            lastReinitMs = now;
            onlineStatus = bme.begin(addr, &Wire);
            return onlineStatus;
        }
        return onlineStatus;
    }

    void updateMqAdcHealth(int mqRaw) {
        // ... (остальной код функции)
        if (mqRaw == 0) {
            if (state.mqSensorOnline) {
                state.mqSensorOnline = false;
            }
        } else {
            if (!state.mqSensorOnline) {
                state.mqSensorOnline = true;
            }
        }
    }

    // ... (остальной код)
    
    void readRealSensors(Readings& r) {
        static unsigned long lastBmeRoomReinitMs = 0;
        static unsigned long lastBmeOutsideReinitMs = 0;

        // Защищенное чтение BME280 (room)
        if (reinitializeBmeIfDue(bmeRoom, BME_ROOM_ADDR, lastBmeRoomReinitMs, state.bmeRoomOnline)) {
            r.tempRoom = bmeRoom.readTemperature();
            r.humRoom = bmeRoom.readHumidity();
            r.pressureRoomMm = bmePressureToSeaLevelMm(bmeRoom.readPressure() / 100.0F, bmeRoom);
        } else {
            // Если датчик недоступен, оставляем предыдущие значения или устанавливаем безопасные значения по умолчанию
            // r.tempRoom, r.humRoom, r.pressureRoomMm остаются без изменений
        }

        // Защищенное чтение BME280 (outside)
        if (reinitializeBmeIfDue(bmeOutside, BME_OUTSIDE_ADDR, lastBmeOutsideReinitMs, state.bmeOutsideOnline)) {
            r.tempOutside = bmeOutside.readTemperature();
            r.humOutside = bmeOutside.readHumidity();
            r.pressureOutsideMm = bmePressureToSeaLevelMm(bmeOutside.readPressure() / 100.0F, bmeOutside);
        } else {
            // Если датчик недоступен, оставляем предыдущие значения или устанавливаем безопасные значения по умолчанию
            // r.tempOutside, r.humOutside, r.pressureOutsideMm остаются без изменений
        }

        // Чтение LDR стандартным аналоговым режимом (делитель напряжения)
        int ldrRaw = analogRead(LDR_PIN);
        r.lightPercent = (int)((ldrRaw / 4095.0f) * 100.0f);

        // int mqRaw = readAnalogEma(MQ135_PIN, state.mqAdcEma, state.mqAdcEmaReady);
        // updateMqAdcHealth(mqRaw);
    }

    void readMockSensors(Readings& r) {
        const float t = millis() / 1000.0f;
        r.tempRoom = 24.5f + sinf(t / 15.0f) * 0.8f;
        r.humRoom = 42.0f + sinf(t / 18.0f) * 4.0f;
        r.pressureRoomMm = 754 + (int)roundf(sinf(t / 30.0f) * 2.0f);
        r.tempOutside = 17.0f + cosf(t / 20.0f) * 1.3f;
        r.humOutside = 58.0f + sinf(t / 16.0f) * 5.0f;
        r.pressureOutsideMm = r.pressureRoomMm;
        r.co2 = 85 + (int)roundf((sinf(t / 12.0f) + 1.0f) * 25.0f);
        r.lightPercent = 55 + (int)roundf((sinf(t / 10.0f) + 1.0f) * 20.0f);
        state.mqSensorOnline = true;
        state.mqReady = true;
    }
}

void initializeSensorHardware() {
    Serial.println("Инициализация сенсоров...");
    Wire.begin();

    // Инициализация BME280 (room) с защитой
    state.bmeRoomOnline = bmeRoom.begin(BME_ROOM_ADDR, &Wire);
    if (state.bmeRoomOnline) {
        Serial.println("BME280 (Room) успешно запущен!");
    } else {
        Serial.println("Предупреждение: BME280 (Room) не найден. Работаем без него.");
    }

    // Инициализация BME280 (outside) с защитой
    state.bmeOutsideOnline = bmeOutside.begin(BME_OUTSIDE_ADDR, &Wire);
    if (state.bmeOutsideOnline) {
        Serial.println("BME280 (Outside) успешно запущен!");
    } else {
        Serial.println("Предупреждение: BME280 (Outside) не найден. Работаем без него.");
    }

    pinMode(MQ135_PIN, INPUT);
    pinMode(LDR_PIN, INPUT); // Стандартный аналоговый режим (без подтяжек)
    Serial.println("Инициализация сенсоров завершена");
}

void readSensors() {
    state.previous = state.current;
    if (USE_MOCK_SENSORS || BRINGUP_FORCE_MOCK_SENSORS) {
        readMockSensors(state.current);
        return;
    }
    readRealSensors(state.current);
}
