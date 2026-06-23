#ifndef READINGS_H
#define READINGS_H

// Структура для хранения одного полного набора показаний датчиков
struct Readings {
    float tempRoom = 0.0f;
    float humRoom = 0.0f;
    int pressureRoomMm = 0;
    int gasAqi = 0;
    float tempOutside = 0.0f;
    float humOutside = 0.0f;
    int pressureOutsideMm = 0;
    int co2 = 0;
    int lightPercent = 0;
    bool relayOn = false;
};

#endif // READINGS_H
