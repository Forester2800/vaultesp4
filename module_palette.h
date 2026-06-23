#pragma once

// Палитра цветов в стиле Fallout
#include "config.h"
#include "state.h"

// Каноничные цвета палитры
#define PALETTE_GREEN_R 0
#define PALETTE_GREEN_G 255
#define PALETTE_GREEN_B 100

#define PALETTE_AMBER_R 255
#define PALETTE_AMBER_G 160
#define PALETTE_AMBER_B 0

#define PALETTE_WHITE_R 200
#define PALETTE_WHITE_G 230
#define PALETTE_WHITE_B 210

// Инициализация пина потенциометра палитры
void initializePalette();

// Обновление палитры на основе потенциометра
void updatePalette();

// Получение текущего цвета палитры в формате RGB565
uint16_t getCurrentPaletteColor();

// Получение текущего значения палитры (0-255)
uint8_t getCurrentPaletteValue();
