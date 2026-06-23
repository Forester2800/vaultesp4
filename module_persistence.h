#ifndef MODULE_PERSISTENCE_H
#define MODULE_PERSISTENCE_H

#include "state.h"
#include <Preferences.h> // <-- Добавил зависимость

// Объявление глобального объекта prefs
extern Preferences prefs;

// Прототипы функций
const char* faultCodeText(FaultCode code);
void saveLastFault(FaultCode code);
void loadLastFault();
void saveMqBurnInState();
void loadMqBurnInState();
void saveMqCalibrationBaseline();
void loadMqCalibrationBaseline();
void updateMqBurnInState();

#endif // MODULE_PERSISTENCE_H
