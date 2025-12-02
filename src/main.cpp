#include <Arduino.h>
#include "logger.h"
#include "display.h"
#include "battery.h"
#include "scale.h"
#include "touchsensor.h"
#include "wifi_manager.h"

Display display;
Battery battery;
Scale scale;
TouchSensor touchSensor;
WiFiManager wifiManager;

unsigned long lastUpdate = 0;
unsigned long lastAnimationUpdate = 0;
const unsigned long UPDATE_INTERVAL = 25;
const unsigned long ANIMATION_INTERVAL = 400;
const unsigned long TARE_DELAY_MS = 1000;
int animationFrame = 0;
bool delayedTarePending = false;
unsigned long delayedTareTime = 0;

void setup() {
    Serial.begin(115200);
    
    Logger::setLevel(LogLevel::Debug);
    Logger::info("Starting scale project");
    
    if (!display.begin()) {
        Logger::error("Display initialization failed");
        return;
    }
    Logger::info("Display initialized");
    
    display.showBootScreen();
    
    if (!scale.begin()) {
        Logger::error("Scale initialization failed");
    } else {
        Logger::info("Scale initialized");
    }
    
    if (!wifiManager.begin()) {
        Logger::error("WiFi Manager initialization failed");
    } else {
        Logger::info("WiFi Manager initialized");
    }
    
    wifiManager.setBattery(&battery);
    wifiManager.setScale(&scale);
}

void loop() {
    unsigned long currentTime = millis();
    
    if (!delayedTarePending && touchSensor.isTarePressed()) {
        delayedTarePending = true;
        delayedTareTime = currentTime + TARE_DELAY_MS;
        Logger::info("Tare pressed, delayed for %d ms", TARE_DELAY_MS);
    }
    
    wifiManager.update();
    
    if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
        if (delayedTarePending && currentTime >= delayedTareTime) {
            delayedTarePending = false;
            scale.tare();
            Logger::info("Tare performed after delay");
        }

        bool batteryDisconnected = battery.isBatteryDisconnected();
        bool isCharging = battery.isCharging();
        uint8_t barCount = battery.getBarCount();
        uint8_t batteryPercent = battery.getPercentage();
        float weight = scale.getWeight();
        const char* timer = "00:00";
        float flowRate = 2.5f;
        bool wifiApMode = wifiManager.isApMode();
        bool wifiConnected = wifiManager.isInitialized();
        
        display.showMainScreen(barCount, batteryPercent, weight, timer, flowRate, isCharging, animationFrame, batteryDisconnected, wifiConnected, wifiApMode, true, delayedTarePending);
        
        lastUpdate = currentTime;
    }
    
    if (currentTime - lastAnimationUpdate >= ANIMATION_INTERVAL) {
        animationFrame = (animationFrame + 1) % 4;
        lastAnimationUpdate = currentTime;
    }
}
