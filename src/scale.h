#ifndef SCALE_H
#define SCALE_H

#include <Arduino.h>
#include <HX711.h>
#include "board_config.h"
#include "logger.h"

class Scale {
private:
    static constexpr float DEFAULT_CALIBRATION_FACTOR = 4466.2f;
    static constexpr unsigned long INIT_TIMEOUT_MS = 5000;
    static constexpr unsigned long INIT_CHECK_INTERVAL_MS = 100;
    static constexpr unsigned long STABILITY_TIMEOUT_MS = 2000;
    static constexpr float STABILITY_THRESHOLD = 0.5f;

    HX711 scale;
    uint8_t doutPin;
    uint8_t sckPin;
    float calibrationFactor;
    bool initialized;

    // Back the web API: weight processing lives in ScaleApp, so getWeight()
    // returns the cached processed value and tare() raises a request the main
    // loop forwards to ScaleApp.
    float cachedWeight = 0.0f;
    bool tareRequested = false;

public:
    Scale(uint8_t dout = HX711_DOUT, uint8_t sck = HX711_SCK, float calibration = DEFAULT_CALIBRATION_FACTOR)
        : doutPin(dout), sckPin(sck), calibrationFactor(calibration), initialized(false) {}

    bool begin() {
        scale.begin(doutPin, sckPin);

        unsigned long startTime = millis();
        while (!scale.is_ready()) {
            if (millis() - startTime > INIT_TIMEOUT_MS) {
                Logger::error("HX711 initialization timeout");
                return false;
            }
            delay(INIT_CHECK_INTERVAL_MS);
        }

        scale.set_scale(calibrationFactor);

        unsigned long stabilityStart = millis();
        float lastReading = 0.0f;
        bool stable = false;
        while (!stable && (millis() - stabilityStart < STABILITY_TIMEOUT_MS)) {
            float currentReading = scale.get_units(3);
            if (abs(currentReading - lastReading) < STABILITY_THRESHOLD && lastReading != 0.0f) {
                stable = true;
            } else {
                lastReading = currentReading;
                delay(100);
            }
        }

        scale.tare();
        initialized = true;
        return true;
    }

    long readRaw() { return initialized ? scale.read() : 0; }
    bool isReady() { return initialized && scale.is_ready(); }

    float getWeight() { return cachedWeight; }
    void setCachedWeight(float weight) { cachedWeight = weight; }

    void tare() { tareRequested = true; }
    bool consumeTareRequest() {
        bool requested = tareRequested;
        tareRequested = false;
        return requested;
    }

    void setCalibrationFactor(float factor) { calibrationFactor = factor; }
    float getCalibrationFactor() { return calibrationFactor; }
};

#endif
