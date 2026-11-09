#ifndef UBIDOTS_MANAGER_H
#define UBIDOTS_MANAGER_H

#include <Arduino.h>

namespace UbidotsManager {
  void begin(const char* token, const char* deviceLabel);
  void task(void* parameter);
}

#endif // !UBIDOTS_MANAGER_H
