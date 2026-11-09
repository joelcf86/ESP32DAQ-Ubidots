#include "SensorManager.h"
#include <DHT.h>

namespace SensorManager {
  static constexpr uint8_t DHT_PIN = 4;
  static constexpr uint8_t TRIG_PIN = 5;
  static constexpr uint8_t ECHO_PIN = 18;

  static constexpr uint8_t DHT_TYPE = DHT11;
  static constexpr uint32_t SAMPLE_TIME = 2000;
  
  static DHT dht(DHT_PIN, DHT_TYPE);
  
  static SensorData sensorData{
    0.0,
    0.0,
    0.0,
    false,
    false,
    0
  };
  
  static portMUX_TYPE dataMux = portMUX_INITIALIZER_UNLOCKED;
  void begin(){
    dht.begin();
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    digitalWrite(TRIG_PIN, LOW);
    Serial.println("SensorManager Inicializado");
  }
  
  static void readDHT(){
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    bool valid = !isnan(temperature) && !isnan(humidity);
    portENTER_CRITICAL(&dataMux);
    sensorData.dhtValid = valid;

    if (valid){
      sensorData.temperature = temperature;
      sensorData.humidity = humidity;
    }
    portEXIT_CRITICAL(&dataMux);
  }

  static void readDistance(){
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
    bool valid = duration > 0;
    float distance = 0.0;
    
    if(valid){
      distance = (duration * 0.0343f) / 2.0f;
    }
    portENTER_CRITICAL(&dataMux);
    sensorData.distanceValid = valid;
    
    if(valid){
      sensorData.distance = distance;
    }
    portEXIT_CRITICAL(&dataMux);
  }
  
  SensorData getData(){
    SensorData snapshot;
    portENTER_CRITICAL(&dataMux);
    snapshot = sensorData;
    portEXIT_CRITICAL(&dataMux);
    return snapshot;
  }
  
  void task(void* parameter){
    TickType_t lastWakeTime = xTaskGetTickCount();
    for(;;){
      readDHT();
      readDistance();
      
      portENTER_CRITICAL(&dataMux);
      sensorData.lastUpdate = millis();
      portEXIT_CRITICAL(&dataMux);
      
      vTaskDelayUntil(
        &lastWakeTime,
        pdMS_TO_TICKS(SAMPLE_TIME)
      );
    }
  }
}
