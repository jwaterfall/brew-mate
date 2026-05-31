#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include "board_config.h"

// Raw reader for the battery sense pins; interpretation lives in
// BatteryProcessor (scale_app.h). Processed values are cached here for the
// web API, pushed in by the main loop.
class Battery {
private:
    uint8_t batteryPin;
    uint8_t vbusPin;
    uint8_t batterySwitchPin;
    float batteryCalibrationFactor;
    float vbusCalibrationFactor;

    // Cached processed state for the web API.
    float cachedVoltage = 0.0f;
    uint8_t cachedPercent = 0;
    bool cachedUsb = false;
    bool cachedDisconnected = false;

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

    void setCached(float voltage, uint8_t percent, bool usb, bool disconnected) {
        cachedVoltage = voltage;
        cachedPercent = percent;
        cachedUsb = usb;
        cachedDisconnected = disconnected;
    }

    float getVoltage() { return cachedVoltage; }
    uint8_t getPercentage() { return cachedPercent; }
    bool isUsbConnected() { return cachedUsb; }
    bool isBatteryDisconnected() { return cachedDisconnected; }

    void setCalibrationFactor(float factor) { batteryCalibrationFactor = factor; }
    float getCalibrationFactor() { return batteryCalibrationFactor; }
    void setVbusCalibrationFactor(float factor) { vbusCalibrationFactor = factor; }
    float getVbusCalibrationFactor() { return vbusCalibrationFactor; }
};

#endif
