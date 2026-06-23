#ifndef MODULE_CONTROLS_H
#define MODULE_CONTROLS_H

#include "state.h"

// --- Function Prototypes ---

// Initializes the button pin.
void initializeControls();

// Reads the button state and handles clicks/holds. Should be called in a loop.
void handleScreenButton();

// Toggles manual ventilation mode on or off.
void setManualVentMode(bool enabled);

// Manually turns the relay on or off for a specified duration.
void setManualRelay(bool on, uint32_t ttlSec);

// Checks if the manual ventilation timeout has expired.
void maintainManualVentTimeout();

// Wakes the device from standby.
void wakeDevice();

// Puts the device into standby mode.
void enterStandby();

#endif // MODULE_CONTROLS_H
