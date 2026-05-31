#pragma once

#include <Arduino.h>
#include "board_config.h"

// Raw reader for the TTP223 touch pins (HIGH == touched). Debounce and edge
// detection live in TouchProcessor (scale_app.h).
class TouchSensor {
private:
    uint8_t tarePin;
    uint8_t powerPin;

public:
    TouchSensor(uint8_t tare = TARE_PIN, uint8_t power = POWER_PIN)
        : tarePin(tare), powerPin(power) {
        pinMode(tarePin, INPUT_PULLDOWN);
        pinMode(powerPin, INPUT_PULLDOWN);
    }

    bool readTareRaw() { return digitalRead(tarePin) == HIGH; }
    bool readPowerRaw() { return digitalRead(powerPin) == HIGH; }
};
