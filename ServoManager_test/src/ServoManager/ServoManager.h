#ifndef SERVO_MANAGER_H
#define SERVO_MANAGER_H

#include <Arduino.h>

namespace ServoManager{
  void begin();
  void setAngle(int angle);
  int getAngle();
  void detach();
}
#endif // !SERVO_MANAGER_H

