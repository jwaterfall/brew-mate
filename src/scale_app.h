#ifndef SCALE_APP_H
#define SCALE_APP_H

// Hardware-agnostic core: turns raw sensor reads into processed weight, battery
// state, timer, flow rate and the resulting display state. All signal
// processing lives here (weight calibration/tare/smoothing, battery voltage->%,
// touch debounce/edges), so the device only does pin reads and actuation. The
// same code runs on-device (standalone) and on the host (dev proxy). Timing is
// injected via nowMs so the host can drive it from the device's millis() stream.

#include <cstdint>

// Raw sensor reads streamed up from the device.
struct RawInputs {
    uint32_t nowMs;
    int32_t weightRaw;        // HX711 raw counts
    float weightCalFactor;    // counts per gram (from device config)
    uint16_t batteryMv;       // analogReadMilliVolts(batteryPin)
    uint16_t vbusMv;          // analogReadMilliVolts(vbusPin)
    float batteryCalFactor;
    float vbusCalFactor;
    bool batterySwitchRaw;    // digitalRead(switchPin): HIGH == connected
    bool tarePinRaw;          // digitalRead(tarePin): HIGH == touched
    bool powerPinRaw;         // digitalRead(powerPin): HIGH == touched
    bool wifiConnected;
    bool wifiApMode;
    bool bluetoothConnected;
    bool immediateTare;       // API/BLE requested an immediate (non-delayed) tare
    char ipStr[16];           // device IP, shown on the info screen
};

// Fully processed battery state.
struct BatteryResult {
    float voltage;
    uint8_t percent;
    uint8_t bars;
    bool charging;
    bool usb;
    bool disconnected;
};

// Everything the display needs to render the main screen.
struct DisplayState {
    uint8_t batteryBarCount;
    uint8_t batteryPercent;
    float weight;
    char timerStr[8];
    float flowRate;
    bool isCharging;
    int animationFrame;
    bool batteryDisconnected;
    bool wifiConnected;
    bool wifiApMode;
    bool bluetoothConnected;
    bool tarePending;
    bool showInfo;            // showing the IP info screen (tare held)
    char ipStr[16];
};

struct ScaleOutputs {
    DisplayState display;
    BatteryResult battery;  // full battery state (for the on-device web API)
    bool playBuzzer;        // a touch/tare was acknowledged this tick
};

// Weight pipeline: calibration, tare offset, deadband, smoothing.
// Operates purely on raw HX711 counts; tare is just "zero is here now".
class WeightProcessor {
public:
    WeightProcessor();
    void addSample(int32_t raw, float calFactor);
    void tare();          // offset = current smoothed reading
    float grams() const;  // calibrated, deadbanded weight

private:
    static constexpr float SMOOTHING = 0.3f;
    static constexpr float DEADBAND_G = 0.3f;

    float rawSmoothed;
    float offset;
    float calFactor;
    bool haveSample;
};

// Battery pipeline: smoothing, divider + calibration, voltage->% curve,
// bar count, USB/charging detection, disconnect detection.
class BatteryProcessor {
public:
    BatteryProcessor();
    BatteryResult process(uint16_t batteryMv, uint16_t vbusMv,
                          float batteryCal, float vbusCal, bool switchRaw);

private:
    static constexpr float DIVIDER_RATIO = 2.0f;
    static constexpr float MIN_VOLTAGE = 3.0f;
    static constexpr float MAX_VOLTAGE = 4.19f;
    static constexpr float SMOOTHING = 0.1f;
    static constexpr float USB_THRESHOLD = 2.5f;

    float batterySmoothed;
    float vbusSmoothed;
    bool haveSample;

    uint8_t percentageFromVoltage(float voltage) const;
};

// Touch pipeline: debounce + edge detection. Triggers on the debounced release
// edge (matching the original on-device behaviour).
class TouchProcessor {
public:
    TouchProcessor();
    bool update(bool rawHigh, uint32_t now);

private:
    static constexpr uint32_t DEBOUNCE_DELAY_MS = 200;

    bool lastState;
    uint32_t lastTime;
};

class ScaleApp {
public:
    ScaleApp();
    ScaleOutputs tick(const RawInputs& in);

private:
    static const uint32_t TARE_DELAY_MS = 1000;
    static const uint32_t INFO_HOLD_MS = 2000;
    static const uint32_t ANIMATION_INTERVAL = 400;
    static const uint32_t FLOW_WINDOW_MS = 1000;
    static const int FLOW_HISTORY = 16;
    static constexpr float FLOW_SMOOTHING = 0.3f;

    WeightProcessor weightProc;
    BatteryProcessor batteryProc;
    TouchProcessor touchTare;
    TouchProcessor touchPower;

    // Timer
    bool timerRunning;
    uint32_t timerStartTime;
    uint32_t timerElapsedTime;

    // Delayed tare (touch)
    bool tarePending;
    uint32_t tareDueTime;

    // Tare-hold -> show IP info screen
    bool tareDown;
    uint32_t tareDownSince;
    bool infoActive;
    bool suppressNextTare;

    // Battery-icon animation
    uint32_t lastAnimationUpdate;
    int animationFrame;

    // Flow-rate estimation (ring buffer of recent weight samples)
    uint32_t flowTime[FLOW_HISTORY];
    float flowWeight[FLOW_HISTORY];
    int flowCount;
    int flowHead;
    float flowRateSmoothed;

    void handlePowerPress(uint32_t now);
    float computeFlowRate(uint32_t now, float weight);
};

#endif
