#pragma once

#include <TFT_eSPI.h>

#include "state.h"

class UIAnimation {
public:
    UIAnimation(TFT_eSPI* tft, int x, int y, int width, int height, unsigned long duration, void (*onComplete)() = nullptr) {
        this->tft = tft;
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
        this->duration = duration;
        this->onComplete = onComplete;
        this->startTime = 0;
        this->progress = 0;
        this->lastStep = -1; // Initialize to an invalid step
    }

    void start() {
        startTime = millis();
    }

    void update(bool forceRedraw = false) {
        unsigned long currentTime = millis();
        progress = (float)(currentTime - startTime) / duration;

        if (progress > 1.0) {
            progress = 1.0;
            if (onComplete) {
                onComplete();
            }
        }
        
        draw(forceRedraw);
    }

    void reset() {
        progress = 0;
        lastStep = -1;
    }

private:
    TFT_eSPI* tft;
    int x, y, width, height;
    unsigned long duration;
    unsigned long startTime;
    float progress;
    void (*onComplete)();
    int lastStep;

    void draw(bool forceRedraw) {
        int steps = 10; // Number of rectangles to draw
        int currentStep = (int)(progress * steps);

        // Redraw only if the step has changed
        if (currentStep != lastStep || forceRedraw) {
            int rectWidth = width / steps;

            for (int i = 0; i < steps; i++) {
                if (i < currentStep) {
                    tft->fillRect(x + i * rectWidth, y, rectWidth, height, TFT_GREEN);
                } else {
                    tft->fillRect(x + i * rectWidth, y, rectWidth, height, TFT_BLACK);
                }
            }
            lastStep = currentStep;
        }
    }
};
