#include "secrets.h"

#include "src/DataManager/DataManager.h"
#include "src/SensorManager/SensorManager.h"
#include "src/EncoderManager/EncoderManager.h"
#include "src/ServoManager/ServoManager.h"
#include "src/WiFiManager/WiFiManager.h"
#include "src/UbidotsManager/UbidotsManager.h"

static constexpr uint32_t CONTROL_INTERVAL = 50;
static constexpr uint32_t STATUS_INTERVAL = 1000;

static uint32_t lastStatus = 0;

static void controlLoop() {
  int remoteSetpoint;

  if (UbidotsManager::takeSetpoint(remoteSetpoint)) {
    EncoderManager::setPosition(remoteSetpoint);
    Serial.printf("Setpoint remoto: %d grados\n", remoteSetpoint);
  }

  if (UbidotsManager::consumeReset()) {
    EncoderManager::reset();
    Serial.println("Reset remoto desde Ubidots");
  }

  int position = EncoderManager::getPosition();

  if (EncoderManager::getButtonState()) {
    EncoderManager::reset();
    position = 0;
    Serial.println("Encoder reiniciado");
  }

  if (position != ServoManager::getAngle()) {
    ServoManager::setAngle(position);
  }

  DataManager::updateEncoder(position);
  DataManager::updateServo(ServoManager::getAngle());

  SensorManager::SensorData sensors = SensorManager::getData();

  DataManager::updateSensors(
    sensors.temperature,
    sensors.humidity,
    sensors.distance,
    sensors.dhtValid,
    sensors.distanceValid
  );
}

static void printStatus() {
  DataManager::SystemData data = DataManager::getData();

  Serial.println("\n--- ESTADO DEL SISTEMA ---");

  if (WiFiManager::isConnected()) {
    Serial.print("WiFi: Conectado | IP: ");
    Serial.println(WiFiManager::getIP());
  } else {
    Serial.println("WiFi: Desconectado");
  }

  Serial.printf(
    "Temp: %.2f C | Hum: %.2f %%HR | Dist: %.2f cm\n",
    data.temperature,
    data.humidity,
    data.distance
  );

  Serial.printf(
    "Encoder: %d grados | Servo: %d grados\n",
    data.encoderPosition,
    data.servoAngle
  );

  Serial.printf(
    "DHT valido: %s | Distancia valida: %s\n",
    data.dhtValid ? "SI" : "NO",
    data.distanceValid ? "SI" : "NO"
  );
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n================================");
  Serial.println("     ESP32 DAQ + UBIDOTS");
  Serial.println("================================");

  DataManager::begin();
  EncoderManager::begin();
  ServoManager::begin();
  SensorManager::begin();

  ServoManager::setAngle(EncoderManager::getPosition());

  WiFiManager::begin(WIFI_SSID, WIFI_PSWD);

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

  UbidotsManager::begin(UBIDOTS_TOKEN, UBIDOTS_DEVICE_LABEL);

  Serial.println("Sistema iniciado");
}

void loop() {
  controlLoop();

  uint32_t now = millis();

  if (now - lastStatus >= STATUS_INTERVAL) {
    lastStatus = now;
    printStatus();
  }

  vTaskDelay(pdMS_TO_TICKS(CONTROL_INTERVAL));
}
