#ifndef TOUCHSENSOR_H
#define TOUCHSENSOR_H

#include <Arduino.h>
#include "board_config.h"

class TouchSensor {
private:
    uint8_t tarePin;
    uint8_t powerPin;
    bool lastTareState;
    bool lastPowerState;
    unsigned long lastTareTime;
    unsigned long lastPowerTime;
    
    static constexpr unsigned long DEBOUNCE_DELAY_MS = 200;

    bool isPressed(uint8_t pin, bool& lastState, unsigned long& lastTime) {
        bool currentState = digitalRead(pin) == HIGH;
        unsigned long currentTime = millis();
        bool pressed = false;
        
        if (currentState != lastState) {
            if (currentTime - lastTime > DEBOUNCE_DELAY_MS) {
                if (!currentState && lastState) {
                    pressed = true;
                }
                lastState = currentState;
                lastTime = currentTime;
            }
        }
        
        return pressed;
    }

public:
    TouchSensor(uint8_t tare = TARE_PIN, uint8_t power = POWER_PIN) 
        : tarePin(tare), powerPin(power), lastTareState(false), lastPowerState(false), lastTareTime(0), lastPowerTime(0) {
        pinMode(tarePin, INPUT_PULLDOWN);
        pinMode(powerPin, INPUT_PULLDOWN);
    }
    
    bool isTarePressed() {
        return isPressed(tarePin, lastTareState, lastTareTime);
    }
    
    bool isPowerPressed() {
        return isPressed(powerPin, lastPowerState, lastPowerTime);
    }
};

#endif
