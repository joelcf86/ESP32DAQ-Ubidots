#include "src/SensorManager/SensorManager.h"
#include "src/WiFiManager/WiFiManager.h"
#include "secrets.h"

void setup() {
  Serial.begin(115200);
  WiFiManager::begin(WIFI_SSID, WIFI_PSWD);
  SensorManager::begin();

  xTaskCreate(
    WiFiManager::task,
    "WiFiTask",
    4096,
    NULL,
    2,
    NULL
  );
  xTaskCreate(
    SensorManager::task,
    "SensorTask",
    4096,
    NULL,
    2,
    NULL
  );
}

void loop() {
  SensorManager::SensorData data = SensorManager::getData();
  if (data.dhtValid){
    Serial.printf(
      "Temperatura: %.1f C  |  Humedad %.1f %%HR\n",
      data.temperature,
      data.humidity
    );
  } else {
    Serial.println("[ERROR Leyendo DHT11]");
  }

  if (data.distanceValid){
    Serial.printf(
      "Distancia HC-SR04: %.2f\n",
      data.distance
    );
  } else {
    Serial.println("[ERROR Leyendo HC-SR04]");
  }

  Serial.println("------------------------------------");
  vTaskDelay(pdMS_TO_TICKS(2000));
}
