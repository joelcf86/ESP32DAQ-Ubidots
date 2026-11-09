#include "src/DataManager/DataManager.h"


void setup() {
  Serial.begin(115200);
  delay(500);
  DataManager::begin();
  DataManager::updateSensors(
    25.5,
    60.2,
    35.8,
    true,
    true
  );
  DataManager::updateEncoder(90);
  DataManager::updateServo(90);
}

void loop() {
    DataManager::SystemData data = DataManager::getData();
    Serial.println("\n --- SYSTEM DATA ---");

    Serial.printf("Temperature: %.2f C\n", data.temperature);
    Serial.printf("Humidity: %.2f %%HR\n", data.humidity);
    Serial.printf("Distance: %.2f cm\n", data.distance);
    
    Serial.printf("Encoder: %d\n", data.encoderPosition);
    Serial.printf("Servo: %d degrees\n", data.servoAngle);

    Serial.printf("DHT Valid: %s\n", data.dhtValid ? "YES" : "NO");
    Serial.printf("Distance Valid: %s\n", data.distanceValid ? "YES" : "NO");

    delay(1000);
}
