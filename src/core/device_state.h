#pragma once

#include <cstdint>

// Shared snapshot of processed values that the web API and BLE read, plus a
// tare request they can raise. The main loop fills this each tick from ScaleApp
// output and consumes the tare request, so the driver classes stay pure raw
// readers with no caching responsibilities.
struct DeviceState {
    float weight = 0.0f;
    uint8_t batteryPercent = 0;
    float batteryVoltage = 0.0f;
    bool usbConnected = false;
    bool batteryDisconnected = false;

    volatile bool tareRequested = false;

    void requestTare() { tareRequested = true; }
    bool consumeTareRequest() {
        bool requested = tareRequested;
        tareRequested = false;
        return requested;
    }
};
