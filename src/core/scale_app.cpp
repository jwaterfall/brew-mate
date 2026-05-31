#include "scale_app.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ----- WeightProcessor -------------------------------------------------------

WeightProcessor::WeightProcessor()
    : rawSmoothed(0.0f), offset(0.0f), calFactor(1.0f), displayed(0.0f), haveSample(false) {}

void WeightProcessor::addSample(int32_t raw, float cal) {
    if (cal != 0.0f) calFactor = cal;
    if (!haveSample) {
        rawSmoothed = (float)raw;
        offset = rawSmoothed;  // zero on first reading
        haveSample = true;
    } else {
        rawSmoothed += SMOOTHING * ((float)raw - rawSmoothed);
    }
}

void WeightProcessor::tare() {
    offset = rawSmoothed;
    displayed = 0.0f;  // snap the screen to zero immediately
}

float WeightProcessor::grams() const {
    if (calFactor == 0.0f) return 0.0f;
    float g = (rawSmoothed - offset) / calFactor;
    if (std::fabs(g) < DEADBAND_G) return 0.0f;
    return g;
}

float WeightProcessor::displayGrams() {
    float g = grams();
    if (std::fabs(g - displayed) >= DISPLAY_HYSTERESIS_G) {
        displayed = g;
    }
    return displayed;
}

// ----- BatteryProcessor ------------------------------------------------------

BatteryProcessor::BatteryProcessor()
    : batterySmoothed(0.0f), vbusSmoothed(0.0f), displayedPercent(0), haveSample(false) {}

uint8_t BatteryProcessor::percentageFromVoltage(float voltage) const {
    if (voltage <= MIN_VOLTAGE) return 0;
    if (voltage >= MAX_VOLTAGE) return 100;

    float percent;
    if (voltage >= 4.0f) {
        percent = 80.0f + ((voltage - 4.0f) / (MAX_VOLTAGE - 4.0f)) * 20.0f;
    } else if (voltage >= 3.7f) {
        percent = 30.0f + ((voltage - 3.7f) / (4.0f - 3.7f)) * 50.0f;
    } else if (voltage >= 3.5f) {
        percent = 10.0f + ((voltage - 3.5f) / (3.7f - 3.5f)) * 20.0f;
    } else {
        percent = ((voltage - MIN_VOLTAGE) / (3.5f - MIN_VOLTAGE)) * 10.0f;
    }

    if (percent > 100.0f) percent = 100.0f;
    if (percent < 0.0f) percent = 0.0f;
    return (uint8_t)percent;
}

BatteryResult BatteryProcessor::process(uint16_t batteryMv, uint16_t vbusMv,
                                        float batteryCal, float vbusCal, bool switchRaw) {
    float battV = (batteryMv / 1000.0f) * DIVIDER_RATIO * batteryCal;
    float vbusV = (vbusMv / 1000.0f) * DIVIDER_RATIO * vbusCal;

    bool firstSample = !haveSample;
    if (firstSample) {
        batterySmoothed = battV;
        vbusSmoothed = vbusV;
        haveSample = true;
    } else {
        batterySmoothed += SMOOTHING * (battV - batterySmoothed);
        vbusSmoothed += SMOOTHING * (vbusV - vbusSmoothed);
    }

    uint8_t pct = percentageFromVoltage(batterySmoothed);
    int delta = (int)pct - (int)displayedPercent;
    if (delta < 0) delta = -delta;
    if (firstSample || delta >= PERCENT_HYSTERESIS) displayedPercent = pct;

    BatteryResult r;
    r.voltage = batterySmoothed;
    r.percent = displayedPercent;
    r.usb = vbusSmoothed > USB_THRESHOLD;
    r.charging = r.usb && batterySmoothed < MAX_VOLTAGE;
    r.disconnected = !switchRaw;

    if (r.percent == 0) r.bars = 0;
    else if (r.percent >= 67) r.bars = 3;
    else if (r.percent >= 34) r.bars = 2;
    else r.bars = 1;

    return r;
}

// ----- TouchProcessor --------------------------------------------------------

TouchProcessor::TouchProcessor() : lastState(false), lastTime(0) {}

bool TouchProcessor::update(bool rawHigh, uint32_t now) {
    bool pressed = false;
    if (rawHigh != lastState) {
        if (now - lastTime > DEBOUNCE_DELAY_MS) {
            if (!rawHigh && lastState) {
                pressed = true;  // debounced release edge
            }
            lastState = rawHigh;
            lastTime = now;
        }
    }
    return pressed;
}

// ----- ScaleApp --------------------------------------------------------------

ScaleApp::ScaleApp()
    : timerRunning(false), timerStartTime(0), timerElapsedTime(0),
      tarePending(false), tareDueTime(0),
      tareDown(false), tareDownSince(0), infoActive(false), suppressNextTare(false),
      lastAnimationUpdate(0), animationFrame(0),
      flowCount(0), flowHead(0), flowRateSmoothed(0.0f) {}

void ScaleApp::handlePowerPress(uint32_t now) {
    if (timerRunning) {
        timerElapsedTime = now - timerStartTime;
        timerRunning = false;
    } else if (timerElapsedTime > 0) {
        timerElapsedTime = 0;
    } else {
        timerStartTime = now - timerElapsedTime;
        timerRunning = true;
    }
}

float ScaleApp::computeFlowRate(uint32_t now, float weight) {
    flowTime[flowHead] = now;
    flowWeight[flowHead] = weight;
    flowHead = (flowHead + 1) % FLOW_HISTORY;
    if (flowCount < FLOW_HISTORY) flowCount++;

    int newestIdx = (flowHead - 1 + FLOW_HISTORY) % FLOW_HISTORY;

    int oldestIdx = -1;
    for (int i = 0; i < flowCount; i++) {
        int idx = (newestIdx - i + FLOW_HISTORY) % FLOW_HISTORY;
        if (now - flowTime[idx] <= FLOW_WINDOW_MS) {
            oldestIdx = idx;
        } else {
            break;
        }
    }

    float rate = 0.0f;
    if (oldestIdx >= 0) {
        uint32_t spanMs = now - flowTime[oldestIdx];
        if (spanMs >= 100) {
            float dw = flowWeight[newestIdx] - flowWeight[oldestIdx];
            rate = dw / (spanMs / 1000.0f);
        }
    }

    flowRateSmoothed += FLOW_SMOOTHING * (rate - flowRateSmoothed);
    if (flowRateSmoothed < 0.0f) flowRateSmoothed = 0.0f;
    return flowRateSmoothed;
}

ScaleOutputs ScaleApp::tick(const RawInputs& in) {
    ScaleOutputs out;
    out.playBuzzer = false;
    uint32_t now = in.nowMs;

    bool tareEdge = touchTare.update(in.tarePinRaw, now);
    bool powerEdge = touchPower.update(in.powerPinRaw, now);

    weightProc.addSample(in.weightRaw, in.weightCalFactor);
    out.battery = batteryProc.process(in.batteryMv, in.vbusMv,
                                      in.batteryCalFactor, in.vbusCalFactor, in.batterySwitchRaw);

    // Hold tare to show the IP info screen; that long hold must not also tare.
    if (in.tarePinRaw) {
        if (!tareDown) { tareDown = true; tareDownSince = now; }
    } else {
        tareDown = false;
    }
    bool showInfo = tareDown && (now - tareDownSince >= INFO_HOLD_MS) &&
                    (in.wifiConnected || in.wifiApMode);
    if (showInfo) {
        if (!infoActive) out.playBuzzer = true;
        suppressNextTare = true;
    }
    infoActive = showInfo;

    if (in.immediateTare) {
        weightProc.tare();
        out.playBuzzer = true;
    }

    if (tareEdge) {
        if (suppressNextTare) {
            suppressNextTare = false;
        } else if (!tarePending) {
            out.playBuzzer = true;
            tarePending = true;
            tareDueTime = now + TARE_DELAY_MS;
        }
    }

    if (powerEdge) {
        out.playBuzzer = true;
        handlePowerPress(now);
    }

    if (tarePending && (int32_t)(now - tareDueTime) >= 0) {
        tarePending = false;
        weightProc.tare();
    }

    float weight = weightProc.grams();
    out.weight = weight;
    float displayWeight = weightProc.displayGrams();

    if (now - lastAnimationUpdate >= ANIMATION_INTERVAL) {
        animationFrame = (animationFrame + 1) % 4;
        lastAnimationUpdate = now;
    }

    uint32_t currentTimer = timerRunning ? (now - timerStartTime) : timerElapsedTime;
    uint32_t seconds = (currentTimer / 1000) % 60;
    uint32_t minutes = (currentTimer / 60000) % 100;  // cap at 99 mins

    DisplayState& d = out.display;
    d.batteryBarCount = out.battery.bars;
    d.batteryPercent = out.battery.percent;
    d.weight = displayWeight;
    snprintf(d.timerStr, sizeof(d.timerStr), "%02u:%02u", (unsigned)minutes, (unsigned)seconds);
    d.flowRate = computeFlowRate(now, weight);
    d.isCharging = out.battery.charging;
    d.animationFrame = animationFrame;
    d.batteryDisconnected = out.battery.disconnected;
    d.wifiConnected = in.wifiConnected;
    d.wifiApMode = in.wifiApMode;
    d.bluetoothConnected = in.bluetoothConnected;
    d.tarePending = tarePending;
    d.showInfo = showInfo;
    strncpy(d.ipStr, in.ipStr, sizeof(d.ipStr));
    d.ipStr[sizeof(d.ipStr) - 1] = '\0';

    return out;
}
