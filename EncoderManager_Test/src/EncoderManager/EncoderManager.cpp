#include "EncoderManager.h"

namespace EncoderManager { 

  static volatile uint8_t encoderState = 0;
  static volatile int8_t encoderCount = 0;

  static const int8_t encoderTable[16] = { 0, -1, 1, 0,
                                             1, 0, 0, -1,
                                            -1, 0, 0, 1,
                                             0, 1, -1, 0 };

  static constexpr uint8_t CLK_PIN = 32;
  static constexpr uint8_t DT_PIN = 33;
  static constexpr uint8_t SW_PIN = 25;

  static constexpr int MIN_POSITION = 0;
  static constexpr int MAX_POSITION = 180;

  static volatile int position = 0;
  static volatile bool buttonPressed = false;

  static  portMUX_TYPE encoderMUX = portMUX_INITIALIZER_UNLOCKED;

  static void IRAM_ATTR encoderISR() {
    uint8_t clk = digitalRead(CLK_PIN);
    uint8_t dt = digitalRead(DT_PIN);

    uint8_t currentState = (clk << 1) | dt;
    portENTER_CRITICAL_ISR(&encoderMUX);
    uint8_t index = (encoderState << 2) | currentState;
    encoderCount += encoderTable[index];
    encoderState = currentState;

    if (encoderCount >= 4){
      if (position < MAX_POSITION){
        position++;
      }
      encoderCount = 0;
    }
    else if (encoderCount <= -4) {
      if(position > MIN_POSITION){
        position--;
      }
      encoderCount = 0;
    }
  portEXIT_CRITICAL_ISR(&encoderMUX);
  }
  
  static void IRAM_ATTR buttonISR(){
    portENTER_CRITICAL_ISR(&encoderMUX);
    buttonPressed = true;
    portEXIT_CRITICAL_ISR(&encoderMUX);
  }

  void begin(){
    pinMode(CLK_PIN, INPUT_PULLUP);
    pinMode(DT_PIN, INPUT_PULLUP);
    pinMode(SW_PIN, INPUT_PULLUP);

    uint8_t clk = digitalRead(CLK_PIN);
    uint8_t dt = digitalRead(DT_PIN);

    uint8_t currentState = (clk << 1) | dt;
    encoderCount = 0;
    encoderState = currentState;

    attachInterrupt(
      digitalPinToInterrupt(CLK_PIN),
      encoderISR,
      CHANGE
    );

     attachInterrupt(
      digitalPinToInterrupt(DT_PIN),
      encoderISR,
      CHANGE
    );

    attachInterrupt(
      digitalPinToInterrupt(SW_PIN),
      buttonISR,
      FALLING
    );
    Serial.println("EncoderManager Inicializado!!!");
  }

  int getPosition(){
    int snapshot;
    portENTER_CRITICAL(&encoderMUX);
    snapshot = position;
    portEXIT_CRITICAL(&encoderMUX);
    return snapshot;
  }

  void setPosition(int newPosition){
    newPosition = constrain(
      newPosition,
      MIN_POSITION,
      MAX_POSITION
    );
    
    portENTER_CRITICAL(&encoderMUX);
    position = newPosition;
    portEXIT_CRITICAL(&encoderMUX);
  }

  bool getButtonState(){
    bool state;
    portENTER_CRITICAL(&encoderMUX);
    state = buttonPressed;
    buttonPressed = false;
    portEXIT_CRITICAL(&encoderMUX);
    return state;
  }

  void reset(){
    portENTER_CRITICAL(&encoderMUX);
    position = 0;
    portEXIT_CRITICAL(&encoderMUX);
  }
}
