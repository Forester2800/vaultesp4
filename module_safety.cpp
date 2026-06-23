
#include "module_safety.h"
#include "config.h"
#include "state.h"
#include "module_ventilation.h"
#include "module_relay_io.h"
#include <esp_task_wdt.h>


void initializeWatchdog() {
    // Сначала деинициализируем watchdog если он уже был инициализирован
    esp_task_wdt_deinit();

    const esp_task_wdt_config_t config = {
        .timeout_ms = WATCHDOG_TIMEOUT_SEC * 1000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
        .trigger_panic = true,
    };
    esp_err_t ret = esp_task_wdt_init(&config);
    if (ret == ESP_OK) {
        esp_task_wdt_add(NULL);
        Serial.println("Watchdog успешно инициализирован");
    } else {
        Serial.printf("Ошибка инициализации watchdog: %d\n", ret);
    }
}

void resetSystemWatchdog() {
    esp_task_wdt_reset();
}

void updateSafetyController() {
    VentDecision decision = decideVentilation();
    decision = applyRelayCycleProtection(decision);
    publishVentDecision(decision);
    setRelayOutput(decision.relayOn);
}
