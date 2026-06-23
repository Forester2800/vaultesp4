#ifndef MODULE_UI_HELPERS_H
#define MODULE_UI_HELPERS_H

#include <TFT_eSPI.h>
#include "state.h"

extern TFT_eSPI tft;

void applyThemeColor();
void writeDisplayBacklight(bool on);
void configureDisplayPower();
void displayWake();
void displaySleep();
void loadCustomFont();
void printAt(int16_t x, int16_t y, const char* text, uint16_t fg, uint16_t bg = BLACK);
void printAtCentered(int16_t y, const char* text, uint16_t fg, uint16_t bg = BLACK);
void clearField(int16_t x, int16_t y, int16_t w, int16_t h);
void drawCrtBackdrop();
void clearScreenWithCrt();
const char* getVentReasonText(VentReason reason);
const char* gasStateText();

#endif // MODULE_UI_HELPERS_H
