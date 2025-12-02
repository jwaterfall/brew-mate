#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include "board_config.h"

class Battery {
private:
    uint8_t batteryPin;
    uint8_t vbusPin;
    uint8_t batterySwitchPin;
    float minVoltage;
    float maxVoltage;
    float batteryDividerRatio;
    float batteryCalibrationFactor;
    float lastBatteryVoltage;
    float lastVbusVoltage;
    unsigned long lastUpdate;

    static constexpr int ADC_SAMPLES = 10;
    static constexpr unsigned long UPDATE_INTERVAL = 100;
    static constexpr float PERCENTAGE_VOLTAGE_4_0 = 4.0f;
    static constexpr float PERCENTAGE_VOLTAGE_3_7 = 3.7f;
    static constexpr float PERCENTAGE_VOLTAGE_3_5 = 3.5f;
    static constexpr uint8_t PERCENTAGE_AT_4_0 = 80;
    static constexpr uint8_t PERCENTAGE_AT_3_7 = 30;
    static constexpr uint8_t PERCENTAGE_AT_3_5 = 10;
    static constexpr uint8_t PERCENTAGE_AT_3_5_RANGE = 20;
    static constexpr uint8_t PERCENTAGE_AT_3_7_RANGE = 50;
    static constexpr uint8_t PERCENTAGE_AT_4_0_RANGE = 20;
    static constexpr uint8_t PERCENTAGE_AT_MAX_RANGE = 20;
    static constexpr uint8_t LOW_BATTERY_THRESHOLD = 20;
    static constexpr uint8_t BATTERY_BAR_COUNT = 3;

    float readVoltage(uint8_t pin, float dividerRatio) {
        uint32_t sum = 0;
        for (int i = 0; i < ADC_SAMPLES; i++) {
            sum += analogReadMilliVolts(pin);
            delayMicroseconds(100);
        }
        float voltageAtPin = (sum / (float)ADC_SAMPLES) / 1000.0f;
        return voltageAtPin * dividerRatio;
    }
    
public:
    Battery(uint8_t batteryPin = BATTERY_PIN, uint8_t vbusPin = VBUS_PIN, uint8_t batterySwitchPin = BATTERY_SWITCH_PIN, float minV = 3.0f, float maxV = 4.19f, float batteryDivider = 2.0f) 
        : batteryPin(batteryPin), vbusPin(vbusPin), batterySwitchPin(batterySwitchPin), minVoltage(minV), maxVoltage(maxV), batteryDividerRatio(batteryDivider), batteryCalibrationFactor(1.0f), lastBatteryVoltage(0.0f), lastVbusVoltage(0.0f), lastUpdate(0) {
        pinMode(batteryPin, INPUT);
        pinMode(vbusPin, INPUT);
        pinMode(batterySwitchPin, INPUT_PULLUP);
    }
    
    float getVoltage() {
        unsigned long currentTime = millis();
        
        if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
            lastBatteryVoltage = readVoltage(batteryPin, batteryDividerRatio) * batteryCalibrationFactor;
            lastVbusVoltage = readVoltage(vbusPin, 2.0f);
            lastUpdate = currentTime;
        }
        
        return lastBatteryVoltage;
    }
    
    uint8_t getPercentage() {
        float voltage = getVoltage();
        
        if (voltage <= minVoltage) return 0;
        if (voltage >= maxVoltage) return 100;
        
        float percent;
        if (voltage >= PERCENTAGE_VOLTAGE_4_0) {
            percent = PERCENTAGE_AT_4_0 + ((voltage - PERCENTAGE_VOLTAGE_4_0) / (maxVoltage - PERCENTAGE_VOLTAGE_4_0)) * PERCENTAGE_AT_MAX_RANGE;
        } else if (voltage >= PERCENTAGE_VOLTAGE_3_7) {
            percent = PERCENTAGE_AT_3_7 + ((voltage - PERCENTAGE_VOLTAGE_3_7) / (PERCENTAGE_VOLTAGE_4_0 - PERCENTAGE_VOLTAGE_3_7)) * PERCENTAGE_AT_3_7_RANGE;
        } else if (voltage >= PERCENTAGE_VOLTAGE_3_5) {
            percent = PERCENTAGE_AT_3_5 + ((voltage - PERCENTAGE_VOLTAGE_3_5) / (PERCENTAGE_VOLTAGE_3_7 - PERCENTAGE_VOLTAGE_3_5)) * PERCENTAGE_AT_3_5_RANGE;
        } else {
            percent = ((voltage - minVoltage) / (PERCENTAGE_VOLTAGE_3_5 - minVoltage)) * PERCENTAGE_AT_3_5;
        }
        
        if (percent > 100.0f) percent = 100.0f;
        if (percent < 0.0f) percent = 0.0f;
        
        return (uint8_t)percent;
    }
    
    uint8_t getBarCount() {
        uint8_t percentage = getPercentage();
        if (percentage == 0) return 0;
        if (percentage >= 67) return 3;
        if (percentage >= 34) return 2;
        return 1;
    }
    
    bool isLow() {
        return getPercentage() < LOW_BATTERY_THRESHOLD;
    }
    
    bool isUsbConnected() {
        getVoltage();
        return lastVbusVoltage > 2.5f;
    }
    
    bool isCharging() {
        return isUsbConnected() && getVoltage() < maxVoltage;
    }
    
    bool isBatteryDisconnected() {
        return digitalRead(batterySwitchPin) == LOW;
    }
    
    void setCalibrationFactor(float factor) {
        batteryCalibrationFactor = factor;
    }
    
    float getCalibrationFactor() {
        return batteryCalibrationFactor;
    }
};

#endif
