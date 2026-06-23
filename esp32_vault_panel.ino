
#include "config.h"
#include "state.h"
#include "module_persistence.h"
#include "module_persistence_pressure.h"
#include "module_sensors.h"
#include "module_buttons.h"
#include "module_display.h"
#include "module_safety.h"
#include "module_serial_bridge.h"
#include "module_environment_logic.h"
#include "module_relay_io.h"
#include "module_time_sync.h"
#include "module_ui_helpers.h"
#include "module_palette.h"
#include <TFT_eSPI.h>
#include <time.h>

// Глобальные экземпляры
AppState state;
Preferences prefs; 
Adafruit_BME280 bmeRoom;    
Adafruit_BME280 bmeOutside; 
TFT_eSPI tft = TFT_eSPI();

void setup() {
    // Ждем стабилизации питания перед началом инициализации
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    Serial.begin(115200);
    vTaskDelay(250 / portTICK_PERIOD_MS);
    Serial.println("=== НАЧАЛО SETUP ===");

    // Инициализация бузера
    pinMode(BUZZER_PIN, OUTPUT);

    // Настройка ШИМ для светодиода режима (GPIO 2) - ESP32 Core V3 API
    ledcAttach(MODE_LED_PIN, MODE_LED_PWM_FREQ, MODE_LED_PWM_BITS);

    // ЭФФЕКТ: ГУЛ ТРАНСФОРМАТОРА (Играет 1 раз при старте)
    tone(BUZZER_PIN, 120); delay(8);
    noTone(BUZZER_PIN);   delay(15);
    tone(BUZZER_PIN, 90);  delay(12);
    noTone(BUZZER_PIN);   delay(100);

    for (int freq = 100; freq < 320; freq += 3) {
        tone(BUZZER_PIN, freq);
        delay(6);
    }
    tone(BUZZER_PIN, 320); delay(150);
    noTone(BUZZER_PIN);

    // МГНОВЕННЫЙ ОПРОС ЦВЕТА ПРИ СТАРТЕ (до инициализации модулей)
    pinMode(33, INPUT); // Потенциометр палитры
    int rawHue = analogRead(33);
    int initialHue = map(rawHue, 0, 4095, 0, 255);
    state.currentHue = initialHue;
    
    // Отправляем boot_start пакет с текущим цветом
    Serial.print("{\"type\":\"boot_start\",\"hue\":");
    Serial.print(initialHue);
    Serial.println("}");

    createSharedStateMux();

    // Инициализация модулей
    initializeSensorHardware();
    initializeToggleSwitches();
    initializeRelay();
    initializeWatchdog();
    initializePalette();

    loadLastFault();
    loadMqBurnInState();
    loadMqCalibrationBaseline();

    // Первый запуск
    readSensors();
    updateEnvironmentLogic();
    updateSafetyController();

    initializeDisplay();
    loadCustomFont();
    redrawCurrentScreen();

    // Чтение потенциометра экранов перед завершением загрузки
    pinMode(32, INPUT); // Потенциометр экранов
    int screenAdc = analogRead(32);
    uint8_t initialScreen = (screenAdc < 1900) ? 0 : 1;
    state.currentScreen = initialScreen;

    // bootComplete будет установлен в bootSequence() после завершения анимации
    
    Serial.println("=== SETUP ЗАВЕРШЕН ===");
}

void loop() {
    static unsigned long lastSensorUpdateMs = 0;
    static unsigned long lastDebugPrintMs = 0;
    static unsigned long loopCounter = 0;
    loopCounter++;

    // Обработка тумблеров
    updateToggleSwitches();

    // Обновление палитры цветов
    updatePalette();

    // NTP/Wi‑Fi синхронизация времени (если включено в config.h)
    maintainTimeSync();

    unsigned long now = millis();

    // Обновление сенсоров и логики с заданной периодичностью
    if (now - lastSensorUpdateMs >= SENSOR_UPDATE_MS) {
        lastSensorUpdateMs = now;

        readSensors();            // 1. Считываем свежие данные с сенсоров
        updateEnvironmentLogic(); // 2. Вычисляем флаги (ночь/день, качество воздуха)
        updateSafetyController(); // 3. Принимаем решение о вентиляции на основе флагов
    }


    // Обновление дисплея
    updateDisplay();

    // Выполняем bootSequence state machine (неблокирующая)
    bootSequence();

    // Работа с серийным портом
    maintainPcBridgeState();
    pumpHistoryDump();

    // Сброс watchdog
    resetSystemWatchdog();

    // Задержка для разгрузки процессора
    vTaskDelay(50 / portTICK_PERIOD_MS);
}
