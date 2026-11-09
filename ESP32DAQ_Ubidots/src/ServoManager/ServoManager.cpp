#include "ServoManager.h"
#include <ESP32Servo.h>

namespace ServoManager {
  static constexpr uint8_t SERVO_PIN = 26;

  static constexpr int MIN_ANGLE = 0;
  static constexpr int MAX_ANGLE = 180;
  static constexpr int MIN_PULSE = 500;
  static constexpr int MAX_PULSE = 2400;
  static int currentAngle = 90;
  static Servo servo;

  void begin(){
    servo.setPeriodHertz(50);
    servo.attach(
      SERVO_PIN,
      MIN_PULSE,
      MAX_PULSE
    );
    setAngle(90);
    Serial.println("ServoManager Inicializado");
  }

  void setAngle(int angle){
    angle = constrain(
      angle,
      MIN_ANGLE,
      MAX_ANGLE
    );
    
    currentAngle = angle;
    servo.write(currentAngle);
  }

  int getAngle(){
    return currentAngle;
  }

  void detach(){
    servo.detach();
  }
  
}
