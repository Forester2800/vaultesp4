#include "module_display.h"
#include "config.h"
#include "state.h"
#include "module_ui_helpers.h"
#include "module_safety.h"
#include "module_buttons.h"
#include "ui_main_screen.h"
#include "ui_forecast_screen.h"
#include "ui_status_screen.h"

// Реализация `maintainTimeSync()` находится в `module_time_sync.h` и собирается в основном `.ino`.
void maintainTimeSync();

// --- Local helper functions from backup ---
namespace {
    void bootDelay(uint32_t durationMs) {
        uint32_t started = millis();
        while (millis() - started < durationMs) {
            maintainTimeSync(); // Keep polling NTP during boot delays
            vTaskDelay(1); // Non-blocking delay for watchdog
        }
    }

    const char* bmeBootStatus(bool online) {
        if (USE_MOCK_SENSORS) return "[ СИМУЛЯЦИЯ ]";
        return online ? "[ ОТКЛИК I2C ]" : "[ НЕТ СВЯЗИ ]";
    }

    const char* analogBootStatus(bool online, bool warmup) {
        if (USE_MOCK_SENSORS) return "[ СИМУЛЯЦИЯ ]";
        if (!online) return "[ НЕТ КАНАЛА ]";
        return warmup ? "[ ПРОГРЕВ ]" : "[ ADC-КАНАЛ ]";
    }

    const char* relayBootStatus(bool online) {
        return online ? "[ КОНТУР ГОТОВ ]" : "[ НЕТ КОНТУРА ]";
    }

    void terminalPrintLine(int16_t x, int16_t &y, const char *text, uint16_t charDelayMs, uint16_t pauseMs, uint8_t lineHeight) {
        tft.setCursor(x, y);
        tft.print(text);
        y += lineHeight;
    }
}

// --- Main Public Functions ---

void setBacklightBrightness(uint8_t percent) {
    if (DISPLAY_BACKLIGHT_PIN < 0) return;
    // Конвертируем процент (0-100) в значение PWM (0-255)
    uint8_t pwmValue = (percent * 255) / 100;
    analogWrite(DISPLAY_BACKLIGHT_PIN, pwmValue);
}

void initializeDisplay() {
    Serial.println("Инициализация дисплея...");

    // Инициализация пина подсветки через PWM для экономии тока
    if (DISPLAY_BACKLIGHT_PIN >= 0) {
        pinMode(DISPLAY_BACKLIGHT_PIN, OUTPUT);
        setBacklightBrightness(DISPLAY_BACKLIGHT_DEFAULT_PERCENT);
        Serial.printf("Подсветка: пин=%d, яркость=%d%%\n", DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_DEFAULT_PERCENT);
    }

    // Пытаемся инициализировать дисплей с защитой от зависания
    unsigned long initStart = millis();

    // Сбрасываем watchdog перед потенциально долгой операцией
    resetSystemWatchdog();

    tft.init();
    resetSystemWatchdog(); // Сброс watchdog после init()

    tft.setRotation(0);
    configureDisplayPower();
    displayWake();

    // Проверяем что дисплей работает (попытка тестового заполнения)
    tft.fillScreen(TFT_BLACK);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    resetSystemWatchdog(); // Сброс watchdog после fillScreen()

    // Дополнительная проверка: пытаемся прочитать ID контроллера дисплея
    uint16_t testColor = tft.color565(255, 0, 0);
    tft.fillRect(0, 0, 1, 1, testColor);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    resetSystemWatchdog(); // Сброс watchdog после теста

    // Если инициализация прошла быстро (менее 2 секунд), считаем дисплей онлайн
    if (millis() - initStart < 2000) {
        state.displayOnline = true;
        Serial.println("Дисплей успешно инициализирован");
        state.bootSequenceState = BOOT_IDLE; // Запускаем state machine через loop()
    } else {
        Serial.println("Предупреждение: TFT дисплей не найден (таймаут). Работаем без него.");
        state.displayOnline = false;
        state.bootSequenceState = BOOT_COMPLETE;
    }
}

void bootSequence() {
    static bool bootCompleteHandled = false;
    
    if (!state.displayOnline) {
        Serial.println("Дисплей недоступен, пропускаем bootSequence");
        state.bootSequenceState = BOOT_COMPLETE;
        return;
    }

    uint32_t currentMillis = millis();

    switch (state.bootSequenceState) {
        case BOOT_IDLE:
            Serial.println("bootSequence: BOOT_IDLE -> BOOT_CRT_POWER_ON");
            currentLedMode = BOOT; // Включаем режим загрузки светодиода
            displayWake();
            applyThemeColor();
            tft.fillScreen(TFT_BLACK);
            state.bootSequenceHeight = 6;
            state.bootSequenceState = BOOT_CRT_POWER_ON;
            state.bootSequenceTimer = currentMillis;
            break;

        case BOOT_CRT_POWER_ON:
            if (currentMillis - state.bootSequenceTimer >= 28) {
                state.bootSequenceTimer = currentMillis;
                tft.drawRect(0, (SCREEN_H / 2) - state.bootSequenceHeight / 2, SCREEN_W, state.bootSequenceHeight, state.activeThemeColor);
                vTaskDelay(1); // yield для watchdog
                state.bootSequenceHeight += 38;
                if (state.bootSequenceHeight > SCREEN_H) {
                    Serial.println("bootSequence: BOOT_CRT_POWER_ON -> BOOT_CLEAR_SCREEN");
                    tft.fillScreen(TFT_BLACK);
                    state.bootSequenceState = BOOT_CLEAR_SCREEN;
                    state.bootSequenceTimer = currentMillis;
                }
            }
            break;

        case BOOT_CLEAR_SCREEN:
            if (currentMillis - state.bootSequenceTimer >= 140) {
                Serial.println("bootSequence: BOOT_CLEAR_SCREEN -> BOOT_PRINT_HEADER");
                tft.setTextColor(state.activeThemeColor, TFT_BLACK);
                tft.setTextSize(1);
                state.bootSequenceY = 8;
                state.bootSequenceState = BOOT_PRINT_HEADER;
            }
            break;

        case BOOT_PRINT_HEADER:
            {
                const char* bootHeader = "УБЕЖИЩЕ 337: ПРОВЕРКА ПОСТА";
                const char* lines[] = {
                    "ТМУ-337 ЦЕНТРАЛЬНЫЙ ПОСТ",
                    "СЛУЖБА АТМОСФЕРЫ, 2026",
                    "",
                    bootHeader,
                    "-------------------------",
                    ""
                };
                
                static int lineIndex = 0;
                if (lineIndex < 6) {
                    terminalPrintLine(5, state.bootSequenceY, lines[lineIndex], strlen(lines[lineIndex]) == 0 ? 10 : 20, strlen(lines[lineIndex]) == 0 ? 100 : 150, 14);
                    vTaskDelay(1); // yield для watchdog
                    lineIndex++;
                } else {
                    Serial.println("bootSequence: BOOT_PRINT_HEADER -> BOOT_PRINT_LINES");
                    lineIndex = 0;
                    state.bootSequenceState = BOOT_PRINT_LINES;
                }
            }
            break;

        case BOOT_PRINT_LINES:
            {
                struct BootDiagnostic {
                    const char* label;
                    const char* status;
                };

                const BootDiagnostic diagnostics[] = {
                    {"- BME280/ВНЕШН", bmeBootStatus(state.bmeOutsideOnline)},
                    {"- BME280/ЖИЛОЙ", bmeBootStatus(state.bmeRoomOnline)},
                    {"- GL5516/СВЕТ", analogBootStatus(state.lightSensorOnline, false)},
                    {"- MQ-135/ВОЗДУХ", analogBootStatus(state.mqSensorOnline, true)},
                    {"- КОНТУР/ПРИТ", relayBootStatus(state.relayModuleOnline)}
                };
                
                static int diagIndex = 0;
                if (diagIndex < 5) {
                    char buffer[64];
                    snprintf(buffer, sizeof(buffer), "%-16s %s", diagnostics[diagIndex].label, diagnostics[diagIndex].status);
                    terminalPrintLine(5, state.bootSequenceY, buffer, 15, 200, 14);
                    vTaskDelay(1); // yield для watchdog
                    diagIndex++;
                } else {
                    Serial.println("bootSequence: BOOT_PRINT_LINES -> BOOT_PRINT_SEPARATOR");
                    diagIndex = 0;
                    state.bootSequenceState = BOOT_PRINT_SEPARATOR;
                    state.bootSequenceTimer = currentMillis;
                }
            }
            break;

        case BOOT_PRINT_SEPARATOR:
            if (currentMillis - state.bootSequenceTimer >= 260) {
                Serial.println("bootSequence: BOOT_PRINT_SEPARATOR -> BOOT_PRINT_INIT_TEXT");
                terminalPrintLine(5, state.bootSequenceY, "-------------------------", 15, 200, 14);
                state.bootSequenceInitIndex = 0;
                state.bootSequenceState = BOOT_PRINT_INIT_TEXT;
                state.bootSequenceTimer = currentMillis;
            }
            break;

        case BOOT_PRINT_INIT_TEXT:
            {
                const char* initText = "ИНИЦИАЛИЗАЦИЯ ПОСТА";
                if (state.bootSequenceInitIndex < strlen(initText)) {
                    if (currentMillis - state.bootSequenceTimer >= 60) {
                        state.bootSequenceTimer = currentMillis;
                        char buffer[64];
                        snprintf(buffer, sizeof(buffer), "%.*s", state.bootSequenceInitIndex + 1, initText);
                        terminalPrintLine(5, state.bootSequenceY, buffer, 20, 60, 14);
                        vTaskDelay(1); // yield для watchdog
                        state.bootSequenceInitIndex++;
                    }
                } else {
                    Serial.println("bootSequence: BOOT_PRINT_INIT_TEXT -> BOOT_PRINT_INIT_DOTS");
                    state.bootSequenceState = BOOT_PRINT_INIT_DOTS;
                    state.bootSequenceTimer = currentMillis;
                }
            }
            break;

        case BOOT_PRINT_INIT_DOTS:
            if (currentMillis - state.bootSequenceTimer >= 650) {
                Serial.println("bootSequence: BOOT_PRINT_INIT_DOTS -> BOOT_PRINT_INIT_OK");
                state.bootSequenceCycle = 0;
                state.bootSequenceDotIndex = 0;
                state.bootSequenceState = BOOT_PRINT_INIT_OK;
                state.bootSequenceTimer = currentMillis;
            }
            break;

        case BOOT_PRINT_INIT_OK:
            {
                const char* dotTexts[] = {"ИНИЦИАЛИЗАЦИЯ ПОСТА.", "ИНИЦИАЛИЗАЦИЯ ПОСТА..", "ИНИЦИАЛИЗАЦИЯ ПОСТА..."};
                const uint32_t dotDelays[] = {200, 200, 300};
                
                if (state.bootSequenceCycle < 3) {
                    if (state.bootSequenceDotIndex < 3) {
                        if (currentMillis - state.bootSequenceTimer >= dotDelays[state.bootSequenceDotIndex]) {
                            state.bootSequenceTimer = currentMillis;
                            terminalPrintLine(5, state.bootSequenceY, dotTexts[state.bootSequenceDotIndex], 20, dotDelays[state.bootSequenceDotIndex], 14);
                            vTaskDelay(1); // yield для watchdog
                            state.bootSequenceDotIndex++;
                        }
                    } else {
                        if (state.bootSequenceCycle < 2) {
                            terminalPrintLine(5, state.bootSequenceY, "ИНИЦИАЛИЗАЦИЯ ПОСТА", 20, 200, 14);
                            state.bootSequenceTimer = currentMillis;
                        }
                        state.bootSequenceDotIndex = 0;
                        state.bootSequenceCycle++;
                    }
                } else {
                    Serial.println("bootSequence: BOOT_PRINT_INIT_OK -> BOOT_COMPLETE");
                    terminalPrintLine(5, state.bootSequenceY, "ИНИЦИАЛИЗАЦИЯ ПОСТА... OK", 20, 900, 14);
                    state.bootSequenceState = BOOT_COMPLETE;
                    state.bootSequenceTimer = currentMillis;
                }
            }
            break;

        case BOOT_COMPLETE:
            if (currentMillis - state.bootSequenceTimer >= 900) {
                if (!bootCompleteHandled) {
                    bootCompleteHandled = true;
                    Serial.println("bootSequence: BOOT_COMPLETE -> OPERATIONAL mode");
                    state.bootComplete = true;
                    redrawCurrentScreen();
                    
                    // Переключаем систему в OPERATIONAL - это запустит чтение тумблера
                    setSystemOperational();
                    
                    // Отправляем status_update с boot_complete
                    Serial.print("{\"type\":\"status_update\",\"hue\":");
                    Serial.print(state.currentHue);
                    Serial.print(",\"screen\":");
                    Serial.print(state.currentScreen);
                    Serial.println(",\"boot_complete\":true}");
                    
                    // НЕ сбрасываем состояние на BOOT_IDLE, чтобы bootSequence не выполнялась снова
                    // state.bootSequenceState остается BOOT_COMPLETE
                }
            }
            break;
    }
}

void showPcBridgeAnimation() {
    if (!state.deviceAwake || !state.bootComplete || !state.displayOnline) return;

    displayWake();
    applyThemeColor();
    clearScreenWithCrt();
    tft.setTextSize(1);
    tft.setTextColor(state.activeThemeColor, TFT_BLACK);
    tft.drawRect(0, 0, SCREEN_W, SCREEN_H, state.activeThemeColor);

    int16_t y = 12;
    terminalPrintLine(8, y, "ЗАПУСК СЕТЕВОЙ МАГИСТРАЛИ...", 30, 200, 16);
    terminalPrintLine(8, y, "ПОИСК АППАРАТНОГО УЗЛА ТМУ ... В СЕТИ", 30, 200, 16);
    terminalPrintLine(8, y, "ЛОКАЛИЗАЦИЯ МОДУЛЯ ... ТМУ-337 ЦЕНТР", 30, 160, 24);

    y+= 16;
    terminalPrintLine(8, y, "[!] ТРЕБУЕТСЯ АВТОРИЗАЦИЯ", 30, 140, 16);
    terminalPrintLine(8, y, "УРОВЕНЬ ДОСТУПА: ПРОТОКОЛ СМОТРИТЕЛЯ", 30, 140, 24);

    y+= 16;
    // Посимвольный вывод ОТПРАВКА ЗАПРОСА СЕССИИ
    const char* sessionText = "ОТПРАВКА ЗАПРОСА СЕССИИ";
    for (int i = 0; i < strlen(sessionText); i++) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.*s", i + 1, sessionText);
        terminalPrintLine(8, y, buffer, 30, 28, 16);
    }
    bootDelay(650);
    
    // Анимация троеточия - три цикла появления точек по одной
    for (int cycle = 0; cycle < 3; cycle++) {
        // Появление точек по одной
        terminalPrintLine(8, y, "ОТПРАВКА ЗАПРОСА СЕССИИ.", 30, 200, 16);
        bootDelay(200);
        terminalPrintLine(8, y, "ОТПРАВКА ЗАПРОСА СЕССИИ..", 30, 200, 16);
        bootDelay(200);
        terminalPrintLine(8, y, "ОТПРАВКА ЗАПРОСА СЕССИИ...", 30, 300, 16);
        bootDelay(300);
        
        // Сброс точек для следующего цикла, кроме последнего
        if (cycle < 2) {
            terminalPrintLine(8, y, "ОТПРАВКА ЗАПРОСА СЕССИИ", 30, 200, 16);
            bootDelay(200);
        }
    }
    
    // Добавляем OK (последние три точки остаются)
    terminalPrintLine(8, y, "ОТПРАВКА ЗАПРОСА СЕССИИ... ОК", 30, 160, 16);
    bootDelay(160);
    
    terminalPrintLine(8, y, "СИНХРОНИЗАЦИЯ АРХИВА АТМОСФЕРЫ ... ОК", 30, 130, 16);
    terminalPrintLine(8, y, "ЗАЩИЩЕННЫЙ КАНАЛ СВЯЗИ ... УСТАНОВЛЕН", 30, 130, 24);

    y+= 16;
    terminalPrintLine(8, y, "ДОБРО ПОЖАЛОВАТЬ В СЕТЬ, СМОТРИТЕЛЬ.", 30, 160, 16);

    bootDelay(2000);
    redrawCurrentScreen();
    
    currentLedMode = IDLE; // Возвращаемся в обычный режим после загрузки
}

void switchScreen() {
    if (!state.deviceAwake || !state.displayOnline) return;
    state.currentScreen = (state.currentScreen + 1) % 3; // 3 screens: Main, Forecast, Status
    redrawCurrentScreen();
}

void redrawCurrentScreen() {
    if (!state.deviceAwake || !state.displayOnline) return;
    clearScreenWithCrt();
    switch (state.currentScreen) {
        case 1: drawForecastScreen(true); break;
        case 2: drawStatusScreen(true); break;
        case 0:
        default: drawMainScreen(true); break;
    }
}

void updateDisplay() {
    if (!state.deviceAwake || state.displaySleeping || !state.displayOnline) return;

    // Handle PC bridge animation
    if (state.pcBridgeAnimationPending) {
        state.pcBridgeAnimationPending = false;
        showPcBridgeAnimation();
        return;
    }

    // Не отрисовываем рабочие экраны до завершения загрузки
    if (!state.bootComplete) return;

    state.frameCounter++;

    // Call the update function for the current screen
    switch (state.currentScreen) {
        case 1: drawForecastScreen(false); break;
        case 2: drawStatusScreen(false); break;
        case 0:
        default: drawMainScreen(false); break;
    }
}
