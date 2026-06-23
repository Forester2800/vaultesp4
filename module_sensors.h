#ifndef MODULE_SENSORS_H
#define MODULE_SENSORS_H

#include <Adafruit_BME280.h>

// Объявляем глобальные экземпляры сенсоров как внешние
extern Adafruit_BME280 bmeRoom;
extern Adafruit_BME280 bmeOutside;

void initializeSensorHardware();
void readSensors();

#endif // MODULE_SENSORS_H
