#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <Arduino.h>

namespace DataManager{
  struct SystemData{
    float temperature;
    float humidity;
    float distance;

    int encoderPosition;
    int servoAngle;
    
    bool dhtValid; 
    bool distanceValid;
  };
  void begin();
  void updateSensors(float temperature,
                    float humidity,
                    float distance,
                    bool dhtValid,
                    bool distanceValid);
  void updateEncoder(int position);
  void updateServo(int angle);
  SystemData getData();
}

#endif // !DATA_MANAGER_H
