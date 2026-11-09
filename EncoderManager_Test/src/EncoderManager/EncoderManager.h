#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H
#include <Arduino.h>

namespace EncoderManager{
  struct EncoderData {
    int position;
    bool buttonPressed;
  };
  
  void begin();
  int getPosition();
  void setPosition(int position);
  bool getButtonState();
  void reset();
}
#endif // !ENCODER_MANAGER

