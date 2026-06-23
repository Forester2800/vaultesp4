#ifndef MODULE_SAFETY_H
#define MODULE_SAFETY_H

#include "state.h"

// Прототипы функций
void initializeWatchdog();
void resetSystemWatchdog();
void updateSafetyController();

#endif // MODULE_SAFETY_H
