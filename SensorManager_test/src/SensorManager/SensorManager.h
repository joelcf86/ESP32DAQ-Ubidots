#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H
#include <Arduino.h>

namespace SensorManager {
  struct SensorData {
    float temperature;
    float humidity;
    float distance;
    bool dhtValid;
    bool distanceValid;
    uint32_t lastUpdate;
  };
  
  void begin();
  void task(void* parameter);
  SensorData getData();
}

#endif // !SENSOR_MANAGER_H
