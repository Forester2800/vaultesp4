#pragma once

// Toggle switches (fixing two-position buttons) for manual control

#include "state.h"
#include "config.h"

// Initialize toggle switches pins
void initializeToggleSwitches();

// Read toggle switches and update state
void updateToggleSwitches();

// Switch system from BOOTING to OPERATIONAL state (called by display module)
void setSystemOperational();
