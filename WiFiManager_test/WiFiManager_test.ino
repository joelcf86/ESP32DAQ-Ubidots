#include "src/WiFiManager/WiFiManager.h"
#include "secrets.h"


void setup() {
  Serial.begin(115200);
  WiFiManager::begin(WIFI_SSID, WIFI_PASSWORD);

  xTaskCreate (
      WiFiManager::task,
      "WiFiTask",
      4096,
      NULL,
      2,
      NULL
  );
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
