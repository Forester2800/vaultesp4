#ifndef MODULE_DISPLAY_H
#define MODULE_DISPLAY_H

#include "state.h"

// Режимы работы зеленого светодиода (GPIO 2)
enum LedMode {IDLE, BOOT, MANUAL};
extern LedMode currentLedMode;

// --- High-Level UI Functions ---

// Initializes the display and runs the boot animation.
void initializeDisplay();

// Sets the backlight brightness (0-100%).
void setBacklightBrightness(uint8_t percent);

// Main UI update function, called in the main loop.
void updateDisplay();

// Switches between the main, forecast, and status screens.
void switchScreen();

// Redraws the entire current screen.
void redrawCurrentScreen();

// Shows the PC connection animation.
void showPcBridgeAnimation();

// --- Low-Level Drawing & Animation ---

// Runs the CRT-style boot-up animation.
void bootSequence();

#endif // MODULE_DISPLAY_H
