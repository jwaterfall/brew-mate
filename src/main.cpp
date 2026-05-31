#include <Arduino.h>
#include <LittleFS.h>
#include "logger.h"
#include "display.h"
#include "battery.h"
#include "scale.h"
#include "touch_sensor.h"
#include "buzzer.h"
#include "config_manager.h"
#include "device_state.h"
#include "scale_app.h"
#include "serial_protocol.h"
#include "wifi_manager.h"
#include "bluetooth_scale.h"

Display display;
Battery battery;
Scale scale;
TouchSensor touchSensor;
Buzzer buzzer;
ScaleApp app;
DeviceState deviceState;
WiFiManager wifiManager;
BluetoothScale bluetoothScale;

const unsigned long MIN_UPDATE_INTERVAL = 50;
unsigned long lastUpdate = 0;

// Dev "tap" proxy mode: entered automatically when the host dev server is
// talking to us, dropped when it goes quiet. The scale is a normal standalone
// device the rest of the time.
const char* HANDSHAKE = "BMTAP";
const unsigned long PROXY_TIMEOUT = 1500;
bool proxyMode = false;
unsigned long lastHostMs = 0;

RawInputs readRawInputs(unsigned long now) {
    // Non-blocking HX711 read: sample only when the chip has data ready,
    // otherwise reuse the last value, so the loop never stalls.
    static long lastRaw = 0;
    if (scale.isReady()) lastRaw = scale.readRaw();

    RawInputs in;
    in.nowMs = now;
    in.weightRaw = lastRaw;
    in.weightCalFactor = scale.getCalibrationFactor();
    in.batteryMv = battery.readBatteryMv();
    in.vbusMv = battery.readVbusMv();
    in.batteryCalFactor = battery.getCalibrationFactor();
    in.vbusCalFactor = battery.getVbusCalibrationFactor();
    in.batterySwitchRaw = battery.readSwitchRaw();
    in.tarePinRaw = touchSensor.readTareRaw();
    in.powerPinRaw = touchSensor.readPowerRaw();
    return in;
}

// Read host traffic: a handshake keepalive keeps us in proxy mode; framebuffer
// lines are blitted to the OLED while proxying.
void drainSerial(unsigned long now) {
    static char buf[1100];
    static size_t len = 0;
    static uint8_t frame[protocol::FRAME_BYTES];
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (len > 0) {
                buf[len] = '\0';
                if (strcmp(buf, HANDSHAKE) == 0) {
                    proxyMode = true;
                    lastHostMs = now;
                } else if (buf[0] == 'F') {
                    bool buzz;
                    if (protocol::decodeFrame(buf, frame, protocol::FRAME_BYTES, buzz)) {
                        proxyMode = true;
                        lastHostMs = now;
                        display.drawFrame(frame);
                        if (buzz) buzzer.playTouchSound();
                    }
                }
                len = 0;
            }
        } else if (len < sizeof(buf) - 1) {
            buf[len++] = c;
        }
    }
}

void setup() {
    // The host streams ~1 KB framebuffer lines; the default 256-byte USB-CDC RX
    // buffer would overflow and corrupt them. Enlarge it before begin().
    Serial.setRxBufferSize(4096);
    Serial.begin(115200);

    Logger::setLevel(LogLevel::Info);
    Logger::info("Starting scale project");

    if (!display.begin()) {
        Logger::error("Display initialization failed");
        return;
    }
    Logger::info("Display initialized");

    display.showBootScreen();

    buzzer.begin();
    Logger::info("Buzzer initialized");

    if (!scale.begin()) {
        Logger::error("Scale initialization failed");
    } else {
        Logger::info("Scale initialized");
    }

    DeviceConfig config = ConfigManager::load();
    battery.setCalibrationFactor(config.battery.batteryCalibrationFactor);
    battery.setVbusCalibrationFactor(config.battery.vbusCalibrationFactor);

    if (!wifiManager.begin()) {
        Logger::error("WiFi Manager initialization failed");
    } else {
        Logger::info("WiFi Manager initialized");
    }

    wifiManager.setBattery(&battery);
    wifiManager.setDeviceState(&deviceState);

    bluetoothScale.begin(&deviceState);
    bluetoothScale.onTare([]() {
        deviceState.requestTare();
    });
    Logger::info("Bluetooth initialized");
}

void loop() {
    unsigned long now = millis();

    wifiManager.update();
    bluetoothScale.update();
    drainSerial(now);

    if (proxyMode && now - lastHostMs > PROXY_TIMEOUT) {
        proxyMode = false;
    }

    if (now - lastUpdate >= MIN_UPDATE_INTERVAL) {
        lastUpdate = now;

        RawInputs in = readRawInputs(now);
        in.wifiConnected = wifiManager.isInitialized();
        in.wifiApMode = wifiManager.isApMode();
        in.bluetoothConnected = bluetoothScale.isConnected();
        in.immediateTare = deviceState.consumeTareRequest();

        String ip = wifiManager.isApMode() ? "192.168.4.1" : wifiManager.getConnectedIP().toString();
        strncpy(in.ipStr, ip.c_str(), sizeof(in.ipStr) - 1);
        in.ipStr[sizeof(in.ipStr) - 1] = '\0';

        // Always run the core so the web API / BLE weight stay live, even while
        // a host is driving the display in proxy mode.
        ScaleOutputs out = app.tick(in);
        deviceState.weight = out.weight;
        deviceState.batteryPercent = out.battery.percent;
        deviceState.batteryVoltage = out.battery.voltage;
        deviceState.usbConnected = out.battery.usb;
        deviceState.batteryDisconnected = out.battery.disconnected;

        if (proxyMode) {
            // Stream raw reads up; the host renders and sends frames back
            // (blitted in drainSerial).
            char line[200];
            protocol::encodeInputs(in, line, sizeof(line));
            Serial.println(line);
        } else {
            if (out.playBuzzer) buzzer.playTouchSound();
            display.showMainScreen(out.display);
        }
    }
}
