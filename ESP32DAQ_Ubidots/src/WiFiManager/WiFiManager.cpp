#include "WiFiManager.h"

namespace WiFiManager {
static String _ssid;
static String _password;

  void begin(const char* ssid, const char* password){
    _ssid = ssid;
    _password = password;
    
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_ssid.c_str(), _password.c_str());
    Serial.println("WiFi Inicializado!!!");
  }

  void task(void* parameter){
    (void)parameter;
    for(;;){
      if (WiFi.status() != WL_CONNECTED){
        Serial.println("Conectando a WiFi...");
        WiFi.begin(_ssid.c_str(), _password.c_str());

        uint32_t start = millis();
        while(WiFi.status() != WL_CONNECTED && millis() - start < 15000){
          Serial.print(".");
          vTaskDelay(pdMS_TO_TICKS(500));
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED){
          Serial.println("WiFi Conectado");
          Serial.print("IP: ");
          Serial.println(WiFi.localIP());
        }else{
          Serial.print("Error WiFi. Estado: ");
          Serial.println(WiFi.status());
        }
      }
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }

  bool isConnected(){
    return WiFi.status() == WL_CONNECTED;
  }

  IPAddress getIP(){
    if (!isConnected()){
      return IPAddress(0, 0, 0, 0);
    }
    return WiFi.localIP();
  }

}
