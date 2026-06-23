
#include "module_environment_logic.h"
#include "config.h"
#include "state.h"
#include <math.h>

namespace {
    // Внутренние переменные для отслеживания состояния с задержками
    unsigned long darkSinceMs = 0;
    unsigned long lightSinceMs = 0;
    unsigned long hotSinceMs = 0;
    unsigned long coolSinceMs = 0;
    unsigned long badAirSinceMs = 0;
    unsigned long criticalAirSinceMs = 0;
    unsigned long goodAirSinceMs = 0;
}

float absoluteHumidity(float tempC, float relativeHumidity) {
    if (!isfinite(tempC) || !isfinite(relativeHumidity) || tempC <= -273.15f) return NAN;
    relativeHumidity = constrain(relativeHumidity, 0.0f, 100.0f);
    float saturation = 6.1094f * expf((17.625f * tempC) / (tempC + 243.04f));
    float vapor = saturation * (relativeHumidity / 100.0f);
    return (216.7f * vapor) / (tempC + 273.15f);
}

// Эта функция будет вызываться в главном цикле (loop)
void updateEnvironmentLogic() {
    unsigned long now = millis();

    // 1. Логика определения Ночь/День с гистерезисом
    if (state.current.lightPercent < LIGHT_NIGHT_ON) {
        if (darkSinceMs == 0) darkSinceMs = now;
        lightSinceMs = 0;
        if (now - darkSinceMs >= LIGHT_CONFIRM_MS) {
            state.nightMode = true;
        }
    } else if (state.current.lightPercent > LIGHT_DAY_OFF) {
        if (lightSinceMs == 0) lightSinceMs = now;
        darkSinceMs = 0;
        if (now - lightSinceMs >= LIGHT_CONFIRM_MS) {
            state.nightMode = false;
        }
    }

    // 2. Логика определения качества воздуха с гистерезисом
    if (state.current.gasAqi > MQ_CRITICAL_ON) {
        if (criticalAirSinceMs == 0) criticalAirSinceMs = now;
        if (now - criticalAirSinceMs >= MQ_CRITICAL_CONFIRM_MS) {
            state.airQualityCritical = true;
            state.airQualityAlarm = true; // Критическое состояние также является тревогой
        }
    } else {
        criticalAirSinceMs = 0;
        state.airQualityCritical = false;

        if (state.current.gasAqi > MQ_BAD_ON) {
            if (badAirSinceMs == 0) badAirSinceMs = now;
            goodAirSinceMs = 0;
            if (now - badAirSinceMs >= MQ_BAD_CONFIRM_MS) {
                state.airQualityAlarm = true;
            }
        } else if (state.current.gasAqi < MQ_GOOD_OFF) {
            if (goodAirSinceMs == 0) goodAirSinceMs = now;
            badAirSinceMs = 0;
            if (now - goodAirSinceMs >= MQ_GOOD_CONFIRM_MS) {
                state.airQualityAlarm = false;
            }
        } else {
            // В "серой зоне" сохраняем текущее состояние
            badAirSinceMs = 0;
            goodAirSinceMs = 0;
        }
    }

    // 3. Логика включения режима охлаждения (cooling latch)
    if (state.current.tempRoom > ROOM_HOT_ON) {
        if (hotSinceMs == 0) hotSinceMs = now;
        coolSinceMs = 0;
        if (now - hotSinceMs >= THERMAL_HOT_CONFIRM_MS) {
            state.coolingLatch = true;
        }
    } else if (state.current.tempRoom < ROOM_COOL_OFF) {
        if (coolSinceMs == 0) coolSinceMs = now;
        hotSinceMs = 0;
        if (now - coolSinceMs >= THERMAL_COOL_CONFIRM_MS) {
            state.coolingLatch = false;
        }
    }
}
