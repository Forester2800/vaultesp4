#include "module_buttons.h"
#include "module_relay_io.h"
#include "module_environment_logic.h"
#include "module_safety.h"
#include "module_display.h"
#include "module_audio.h"
#include "module_led_breathing.h"

LedMode currentLedMode = IDLE;

namespace {
    // Состояния системы (State Machine)
    enum SystemState {
        STATE_BOOTING,
        STATE_OPERATIONAL
    };
    
    SystemState systemState = STATE_BOOTING;
    
    bool lastModeActive = false;
    bool lastFanActive = false;
    unsigned long lastBootLedUpdate = 0;
    unsigned long lastPotRead = 0;
    int lastHueValue = 0;
    float filteredHue = 0.0;
    int lastSentHue = -1;
}

void initializeToggleSwitches() {
    // Явно настраиваем цифровые пины пульта управления
    pinMode(TOGGLE_MODE_PIN, INPUT_PULLUP);  // Тумблер режима (GPIO 16)
    pinMode(TOGGLE_FAN_PIN, INPUT_PULLUP);   // Тумблер вентилятора (GPIO 17)
    pinMode(MODE_LED_PIN, OUTPUT);          // Зеленый светодиод режима (GPIO 2)

    // Безопасная инициализация аналоговых входов для потенциометров
    pinMode(33, INPUT); // Потенциометр палитры
    pinMode(32, INPUT); // Потенциометр экранов

    // Инициализация state machine
    systemState = STATE_BOOTING;
    currentLedMode = BOOT; // Начинаем с режима загрузки
    
    // Считываем стартовые состояния тумблеров (но не применяем до конца загрузки)
    lastModeActive = (digitalRead(TOGGLE_MODE_PIN) == LOW);
    lastFanActive = (digitalRead(TOGGLE_FAN_PIN) == LOW);
}

void setSystemOperational() {
    systemState = STATE_OPERATIONAL;
}

void updateToggleSwitches() {
    unsigned long currentMillis = millis();
    
    // --- STATE MACHINE: ПЕРЕХОД ИЗ BOOT В OPERATIONAL ---
    // Переход происходит по внешнему флагу (функция экрана переключит systemState)
    if (systemState == STATE_BOOTING) {
        // Светодиод хаотично мерцает пока в STATE_BOOTING
        if (currentMillis - lastBootLedUpdate >= random(50, 100)) {
            lastBootLedUpdate = currentMillis;
            LedBreathing::setBrightness(random(0, 2) ? 4095 : 0);
        }
        // В BOOT не обрабатываем тумблеры
        return;
    }
    
    // --- ПЕРЕХОД В OPERATIONAL (вызывается функцией экрана) ---
    // Эта часть выполняется когда systemState переключается в STATE_OPERATIONAL
    static bool operationalTransitionHandled = false;
    if (systemState == STATE_OPERATIONAL && !operationalTransitionHandled) {
        operationalTransitionHandled = true;
        currentLedMode = IDLE; // Сбрасываем режим загрузки
        
        // Жестко считываем актуальное состояние тумблера ПРИ ПЕРЕХОДЕ В РАБОЧИЙ РЕЖИМ
        bool isModeActive = (digitalRead(TOGGLE_MODE_PIN) == LOW);
        bool isFanActive = (digitalRead(TOGGLE_FAN_PIN) == LOW);
        
        // Применяем состояние немедленно
        lastModeActive = isModeActive;
        lastFanActive = isFanActive;
        state.manualModeActive = isModeActive;
        
        if (isModeActive) {
            currentLedMode = MANUAL;
            state.manualRelayOn = isFanActive;
            setRelayOutput(isFanActive);
            Serial.print("{\"type\":\"status_update\",\"manual_mode\":true,\"relay_on\":");
            Serial.print(isFanActive ? "true" : "false");
            Serial.println("}");
        } else {
            currentLedMode = IDLE;
            updateEnvironmentLogic();
            Serial.print("{\"type\":\"status_update\",\"manual_mode\":false,\"relay_on\":");
            Serial.print(digitalRead(RELAY_PIN) == HIGH ? "true" : "false");
            Serial.println("}");
        }
    }
    
    // --- ЧТЕНИЕ ТУМБЛЕРОВ (НЕБЛОКИРУЮЩЕЕ) ---
    bool isModeActive = (digitalRead(TOGGLE_MODE_PIN) == LOW); 
    bool isFanActive = (digitalRead(TOGGLE_FAN_PIN) == LOW);

    bool modeChanged = (isModeActive != lastModeActive);
    bool fanChanged = (isFanActive != lastFanActive);

    // В STATE_OPERATIONAL всегда обновляем состояние
    if (systemState == STATE_OPERATIONAL) {
        state.manualModeActive = isModeActive;
    }

    // --- АНАЛОГОВОЕ ЧТЕНИЕ ПОТЕНЦИОМЕТРОВ С ЗАЩИТОЙ ОТ КЛИНА АЦП ---
    if (currentMillis - lastPotRead >= 50) { // 50 мс сделает отклик очень отзывчивым
        lastPotRead = currentMillis;

        // Читаем потенциометр ЦВЕТА (GPIO 33)
        int rawAdc = analogRead(33);

        // Масштабируем в диапазон FastLED (0...255)
        int targetHue = map(rawAdc, 0, 4095, 0, 255);

        // Фильтр Гайвера (экспоненциальное сглаживание).
        // Коэффициент 0.2 дает идеальный баланс: убирает дрожание, но не создает задержки
        filteredHue = (filteredHue * 0.8) + (targetHue * 0.2);

        int currentHue = (int)filteredHue;

        // Отправляем в Serial, если значение изменилось ХОТЯ БЫ на 1 единицу
        if (currentHue != lastSentHue) {
            lastSentHue = currentHue;
            Serial.print("{\"type\":\"status_update\",\"hue\":");
            Serial.print(currentHue);
            Serial.println("}");
        }

        // Читаем потенциометр ЭКРАНОВ (GPIO 32)
        int screenAdc = analogRead(32);
        uint8_t newScreen = state.currentScreen;
        if (screenAdc < 1900) {
            newScreen = 0; // Основной экран
        } else if (screenAdc > 2200) {
            newScreen = 1; // Прогноз погоды
        }
        // Мертвая зона 1900-2200: сохраняем предыдущее состояние

        if (newScreen != state.currentScreen) {
            state.currentScreen = newScreen;
            // Звуковой эффект при переключении экранов
            Audio::playBeepSwitch();
            // Отправляем пакет на сервер только ОДИН раз при переключении
            Serial.print("{\"type\":\"status_update\",\"screen\":");
            Serial.print(state.currentScreen);
            Serial.println("}");
        }
        // redrawCurrentScreen() НЕ вызываем здесь - отрисовка в updateDisplay()
    }

    // --- УПРАВЛЕНИЕ ЗЕЛЕНЫМ СВЕТОДИОДОМ (GPIO 2) ---
    switch (currentLedMode) {
        case BOOT:
            // Имитация работы дисковода - хаотичные короткие вспышки
            if (currentMillis - lastBootLedUpdate >= random(50, 100)) {
                lastBootLedUpdate = currentMillis;
                LedBreathing::setBrightness(random(0, 2) ? 4095 : 0);
            }
            break;
        case IDLE:
            // Дыхание обновляется в основном loop() через LedBreathing::update()
            // Здесь ничего не делаем - это дает полный контроль модулю
            break;
        case MANUAL:
            // В ручном режиме горит постоянно на полной яркости
            LedBreathing::setBrightness(4095);
            break;
    }

    // --- СИНХРОНИЗАЦИЯ РЕЖИМОВ И РЕЛЕ С СЕРВЕРОМ (ТОЛЬКО В OPERATIONAL) ---
    if (systemState != STATE_OPERATIONAL) {
        lastModeActive = isModeActive;
        lastFanActive = isFanActive;
        return;
    }
    
    if (!modeChanged && !fanChanged) {
        lastModeActive = isModeActive;
        lastFanActive = isFanActive;
        return;
    }

    if (!isModeActive) {
        if (modeChanged) {
            currentLedMode = IDLE; // Возвращаем светодиод в режим IDLE
            updateEnvironmentLogic(); // Возвращаем управление автоматике
            // Звуковой эффект при возврате в автоматический режим
            Audio::playBeepAutoMode();
            Serial.print("{\"type\":\"status_update\",\"manual_mode\":false,\"relay_on\":");
            Serial.print(digitalRead(RELAY_PIN) == HIGH ? "true" : "false");
            Serial.println("}");
        }
    } else {
        if (modeChanged || fanChanged) {
            currentLedMode = MANUAL; // Включаем светодиод в режиме MANUAL
            state.manualRelayOn = isFanActive;
            setRelayOutput(isFanActive); // Включаем/выключаем силовой моторчик окна

            // Звуковой эффект при активации ручного режима
            if (modeChanged) {
                Audio::playBeepManualMode();
            }

            Serial.print("{\"type\":\"status_update\",\"manual_mode\":true,\"relay_on\":");
            Serial.print(isFanActive ? "true" : "false");
            Serial.println("}");
        }
    }

    lastModeActive = isModeActive;
    lastFanActive = isFanActive;
}
