#include "src/ServoManager/ServoManager.h"

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Iniciando prueba SG90");
  ServoManager::begin();
}

void loop() {
  ServoManager::setAngle(0);
  Serial.println("Servo: 0 grados");
  delay(1500);
  
  ServoManager::setAngle(90);
  Serial.println("Servo: 90 grados");
  delay(1500);

  ServoManager::setAngle(180);
  Serial.println("Servo: 180 grados");
  delay(1500);
}
