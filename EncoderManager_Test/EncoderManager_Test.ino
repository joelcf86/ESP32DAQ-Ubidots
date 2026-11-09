#include "src/EncoderManager/EncoderManager.h"

void setup() {
  Serial.begin(115200);
  delay(500);

  EncoderManager:: begin();

}

void loop() {
  int position = EncoderManager::getPosition();
  Serial.printf(
    "Encoder Position: %d degrees\n",
    position
  );
  if (EncoderManager::getButtonState()){
    EncoderManager::reset();
    Serial.println("Encoder Reset");
  }
  vTaskDelay(pdMS_TO_TICKS(200));
}
