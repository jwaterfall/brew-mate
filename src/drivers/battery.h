#pragma once

#include <Arduino.h>
#include "board_config.h"

// Raw reader for the battery sense pins. Interpretation (averaging, divider,
// calibration, voltage->% curve, USB/charging/disconnect detection) lives in
// BatteryProcessor (scale_app.h). The calibration factors are stored here so
// they can be persisted and passed into that processing.
class Battery {
private:
    uint8_t batteryPin;
    uint8_t vbusPin;
    uint8_t batterySwitchPin;
    float batteryCalibrationFactor;
    float vbusCalibrationFactor;

public:
    Battery(uint8_t batteryPin = BATTERY_PIN, uint8_t vbusPin = VBUS_PIN, uint8_t batterySwitchPin = BATTERY_SWITCH_PIN)
        : batteryPin(batteryPin), vbusPin(vbusPin), batterySwitchPin(batterySwitchPin),
          batteryCalibrationFactor(1.0f), vbusCalibrationFactor(1.0f) {
        pinMode(batteryPin, INPUT);
        pinMode(vbusPin, INPUT);
        pinMode(batterySwitchPin, INPUT_PULLUP);
    }

    uint16_t readBatteryMv() { return (uint16_t)analogReadMilliVolts(batteryPin); }
    uint16_t readVbusMv() { return (uint16_t)analogReadMilliVolts(vbusPin); }
    bool readSwitchRaw() { return digitalRead(batterySwitchPin) == HIGH; }

    void setCalibrationFactor(float factor) { batteryCalibrationFactor = factor; }
    float getCalibrationFactor() { return batteryCalibrationFactor; }
    void setVbusCalibrationFactor(float factor) { vbusCalibrationFactor = factor; }
    float getVbusCalibrationFactor() { return vbusCalibrationFactor; }
};
