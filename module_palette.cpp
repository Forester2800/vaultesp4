#include "module_palette.h"
#include <math.h>

namespace {
    uint8_t currentPaletteValue = 0;
    uint16_t currentPaletteColor = 0x07E8; // По умолчанию зеленый
    unsigned long lastPaletteUpdate = 0;
}

// Конвертация RGB в RGB565
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Линейная интерполяция между двумя значениями
uint8_t lerp(uint8_t a, uint8_t b, float t) {
    return (uint8_t)(a + (b - a) * t);
}

// 7 точек градиента для плавного перехода цветов
struct ColorPoint {
    uint8_t r, g, b;
};

const ColorPoint PALETTE_POINTS[7] = {
    {0, 255, 100},    // 0: Green
    {0, 255, 255},    // 42: Cyan
    {0, 100, 255},    // 85: Blue
    {150, 0, 255},    // 128: Purple
    {255, 160, 0},    // 170: Amber
    {255, 50, 50},    // 213: Red
    {200, 230, 210}    // 255: White
};

// Получение цвета палитры на основе градиента с 7 точками
uint16_t getPaletteColor(uint8_t value) {
    uint8_t r, g, b;
    float t;
    int segment = value / 36; // 256 / 7 ≈ 36
    int segmentOffset = value % 36;
    
    if (segment >= 6) {
        // Последний сегмент (Red → White)
        segment = 5;
        segmentOffset = 36;
    }
    
    t = segmentOffset / 36.0f;
    
    const ColorPoint& start = PALETTE_POINTS[segment];
    const ColorPoint& end = PALETTE_POINTS[segment + 1];
    
    r = lerp(start.r, end.r, t);
    g = lerp(start.g, end.g, t);
    b = lerp(start.b, end.b, t);
    
    return rgb565(r, g, b);
}

void initializePalette() {
    pinMode(PALETTE_PIN, INPUT);
    currentPaletteValue = 0;
    currentPaletteColor = getPaletteColor(0);
}

void updatePalette() {
    unsigned long now = millis();
    if (now - lastPaletteUpdate < 50) { // Обновляем не чаще чем каждые 50 мс
        return;
    }
    lastPaletteUpdate = now;
    
    int rawAdc = analogRead(PALETTE_PIN);
    uint8_t newValue = (uint8_t)map(rawAdc, 0, 4095, 0, 255);
    
    if (newValue != currentPaletteValue) {
        currentPaletteValue = newValue;
        currentPaletteColor = getPaletteColor(currentPaletteValue);
    }
}

uint16_t getCurrentPaletteColor() {
    return currentPaletteColor;
}

uint8_t getCurrentPaletteValue() {
    return currentPaletteValue;
}
