#include "DataManager.h"

namespace DataManager{
  static SystemData systemData = {0};
  static SemaphoreHandle_t dataMutex = NULL;

  void begin(){
    if (dataMutex != NULL) return;
    dataMutex = xSemaphoreCreateMutex();
    if (dataMutex == NULL){
      Serial.println("ERROR: no se pudo crear el mutex");
      return;
    }
    Serial.println("DataManager Inicializado");
  }
  void updateSensors(float temperature, float humidity,
                     float distance, bool dhtValid, bool distanceValid){

    if (dataMutex == NULL) return;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE){
      systemData.temperature = temperature;
      systemData.humidity = humidity;
      systemData.distance = distance;
      
      systemData.dhtValid = dhtValid;
      systemData.distanceValid = distanceValid;
      
      xSemaphoreGive(dataMutex);
    }
  }

  void updateEncoder(int position){
    if (dataMutex == NULL) return;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)==pdTRUE){
      systemData.encoderPosition = position;
      xSemaphoreGive(dataMutex);
    }
  }

  void updateServo(int angle){
    if (dataMutex == NULL) return;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)==pdTRUE){
      systemData.servoAngle = angle;
      xSemaphoreGive(dataMutex);
    }
  }
  
  SystemData getData(){
    SystemData snapshot = {0};
    if (dataMutex == NULL) return snapshot;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)==pdTRUE){
      snapshot = systemData;
      xSemaphoreGive(dataMutex);
    }
    return snapshot;
  }
}
