#ifndef MODULE_LED_BREATHING_H
#define MODULE_LED_BREATHING_H

#include "config.h"
#include <math.h>

// ============================================================================
// МОДУЛЬ LED BREATHING: Плавное дыхание зелёного светодиода режима
// ============================================================================
// Алгоритм: Экспоненциальный синус (AlexGyver) для "ламповой" эстетики
// Пин: GPIO 2 (MODE_LED_PIN) - 12-битное разрешение ШИМ (0-4095)
// Частота: 5 кГц (бесшумная, комфортная для глаза)
// ============================================================================
// 
// Почему экспоненциальный синус лучше обычного?
// - Обычный синус: линейный подъем и падение → "китайская гирлянда"
// - Экспоненциальный: быстрый подъем, медленное затухание → живое дыхание
// - Результат: выглядит как ламповый индикатор старого оборудования
// ============================================================================

namespace LedBreathing {

    // Внутренние переменные состояния
    namespace {
        unsigned long lastLedUpdate = 0;
        const unsigned int UPDATE_INTERVAL_MS = 15;  // 15 мс между обновлениями (67 FPS)
        const unsigned int BREATH_PERIOD_MS = 2800;  // Полный цикл дыхания (в мс)
        
        // Диапазон яркости (12-бит: 0-4095)
        const int BRIGHTNESS_MIN = 100;   // Не совсем черный (видны "нижние биения")
        const int BRIGHTNESS_MAX = 3500;  // Не максимум (защита для долгой работы LED)
    }

    // ========================================================================
    // CALCULATE BRIGHTNESS: Вычисление яркости на текущий момент
    // ========================================================================
    // Возвращает: значение PWM (0-4095) для текущей фазы дыхания
    // Внутренняя функция (используется в update)
    inline int calculateBrightness(unsigned long elapsed_ms) {
        // Преобразуем миллисекунды в полные обороты (0...2π)
        float phase = (2.0f * M_PI * elapsed_ms) / BREATH_PERIOD_MS;
        
        // Экспоненциальный синус - ВОЛШЕБНАЯ ФОРМУЛА
        // exp(sin(x)) дает характерное "дыхание": быстрый подъем, медленное падение
        float exponential_sine = exp(sin(phase - M_PI / 2.0f));
        
        // Масштабируем в диапазон яркости
        // Диапазон exp(sin(x)): [e^-1, e^1] ≈ [0.368, 2.718]
        // Нормализуем: (value - e^-1) / (e^1 - e^-1) → получаем 0...1
        float normalized = (exponential_sine - 0.36787944f) / (2.71828183f - 0.36787944f);
        
        // Ограничиваем диапазон и масштабируем под наш BRIGHTNESS_MIN/MAX
        normalized = constrain(normalized, 0.0f, 1.0f);
        int brightness = (int)(BRIGHTNESS_MIN + normalized * (BRIGHTNESS_MAX - BRIGHTNESS_MIN));
        
        return brightness;
    }

    // ========================================================================
    // UPDATE: Главная функция обновления LED
    // ========================================================================
    // Вызывается: В loop() регулярно (неблокирующая)
    // Обновляет: Яркость LED в режиме IDLE
    inline void update(unsigned long current_millis, LedMode current_mode) {
        // Обновляем LED только в режиме IDLE (автоматический режим)
        if (current_mode != IDLE) {
            return;
        }
        
        // Неблокирующая проверка времени
        if (current_millis - lastLedUpdate < UPDATE_INTERVAL_MS) {
            return;
        }
        lastLedUpdate = current_millis;
        
        // Вычисляем положение в цикле дыхания
        unsigned long elapsed = current_millis % BREATH_PERIOD_MS;
        
        // Вычисляем яркость и отправляем на пин
        int brightness = calculateBrightness(elapsed);
        ledcWrite(MODE_LED_PIN, brightness);
    }

    // ========================================================================
    // SET BRIGHTNESS DIRECT: Прямое управление яркостью
    // ========================================================================
    // Используется: В режиме BOOT (хаотичное мерцание) и MANUAL (полная яркость)
    // Параметры:
    //   brightness: 0-4095 (0 = выключено, 4095 = максимум)
    inline void setBrightness(int brightness) {
        brightness = constrain(brightness, 0, 4095);
        ledcWrite(MODE_LED_PIN, brightness);
    }

    // ========================================================================
    // GET CURRENT BRIGHTNESS: Получить текущую рассчитанную яркость
    // ========================================================================
    // Используется: Для мониторинга, логирования, дебага
    // Возвращает: Последний вычисленный уровень яркости
    inline int getCurrentBrightness(unsigned long current_millis, LedMode current_mode) {
        if (current_mode != IDLE) {
            return 0;
        }
        unsigned long elapsed = current_millis % BREATH_PERIOD_MS;
        return calculateBrightness(elapsed);
    }

} // namespace LedBreathing

#endif // MODULE_LED_BREATHING_H
