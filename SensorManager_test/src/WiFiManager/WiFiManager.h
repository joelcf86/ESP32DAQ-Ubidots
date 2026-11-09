#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

namespace WiFiManager {
  void begin(const char* ssid, const char* password);
  void task(void* parameter);
  bool isConnected();
  IPAddress getIP();
}

#endif // !WIFI_MANAGER_H
