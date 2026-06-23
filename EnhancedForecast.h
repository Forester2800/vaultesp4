#ifndef ENHANCED_FORECAST_H
#define ENHANCED_FORECAST_H

#include <stdint.h>

// Структура для хранения расширенного прогноза погоды
struct EnhancedForecast {
    uint32_t dt;         // Время прогноза (Unix timestamp)
    float temp;          // Температура
    float feels_like;    // Ощущаемая температура
    int pressure;        // Давление (в гПа)
    int humidity;        // Влажность (в %)
    float dew_point;     // Точка росы
    float uvi;           // УФ-индекс
    int clouds;          // Облачность (в %)
    int visibility;      // Видимость (в метрах)
    float wind_speed;    // Скорость ветра (в м/с)
    int wind_deg;        // Направление ветра (в градусах)
    float wind_gust;     // Порывы ветра (в м/с)
    int weather_id;      // ID погодных условий
};

#endif // ENHANCED_FORECAST_H
