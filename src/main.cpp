#include <Arduino.h>
#include "logger.h"
#include "display.h"
#include "battery.h"
#include "scale.h"
#include "touchsensor.h"
#include "buzzer.h"
#include "wifi_manager.h"
#include "config_manager.h"
#include "bluetooth_scale.h"

Display display;
Battery battery;
Scale scale;
TouchSensor touchSensor;
Buzzer buzzer;
WiFiManager wifiManager;
BluetoothScale bluetoothScale;

unsigned long lastUpdate = 0;
unsigned long lastAnimationUpdate = 0;
const unsigned long MIN_UPDATE_INTERVAL = 50;
const unsigned long ANIMATION_INTERVAL = 400;
const unsigned long TARE_DELAY_MS = 1000;
int animationFrame = 0;
bool delayedTarePending = false;
unsigned long delayedTareTime = 0;

// Timer variables
unsigned long timerStartTime = 0;
unsigned long timerElapsedTime = 0;
bool isTimerRunning = false;

void startTimer() {
    if (!isTimerRunning) {
        timerStartTime = millis() - timerElapsedTime;
        isTimerRunning = true;
    }
}

void stopTimer() {
    if (isTimerRunning) {
        timerElapsedTime = millis() - timerStartTime;
        isTimerRunning = false;
    }
}

void resetTimer() {
    isTimerRunning = false;
    timerElapsedTime = 0;
}

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
    
    buzzer.begin();
    Logger::info("Buzzer initialized");
    
    if (!scale.begin()) {
        Logger::error("Scale initialization failed");
    } else {
        Logger::info("Scale initialized");
    }
    
    // Load battery calibration from config
    DeviceConfig config = ConfigManager::load();
    battery.setCalibrationFactor(config.battery.batteryCalibrationFactor);
    battery.setVbusCalibrationFactor(config.battery.vbusCalibrationFactor);
    
    if (!wifiManager.begin()) {
        Logger::error("WiFi Manager initialization failed");
    } else {
        Logger::info("WiFi Manager initialized");
    }
    
    // Set battery and scale references after WiFiManager is initialized
    wifiManager.setBattery(&battery);
    wifiManager.setScale(&scale);

    // Initialize Bluetooth
    bluetoothScale.begin(&scale);
    bluetoothScale.onTare([]() {
        scale.tare();
        buzzer.playTouchSound();
    });
    Logger::info("Bluetooth initialized");
}

void loop() {
    unsigned long currentTime = millis();
    
    if (!delayedTarePending && touchSensor.isTarePressed()) {
        buzzer.playTouchSound();
        delayedTarePending = true;
        delayedTareTime = currentTime + TARE_DELAY_MS;
        Logger::info("Tare pressed, delayed for %d ms", TARE_DELAY_MS);
    }
    
    if (touchSensor.isPowerPressed()) {
        buzzer.playTouchSound();
        if (isTimerRunning) {
            stopTimer();
        } else if (timerElapsedTime > 0) {
            resetTimer();
        } else {
            startTimer();
        }
    }
    
    wifiManager.update();
    bluetoothScale.update();
    
    if (currentTime - lastUpdate >= MIN_UPDATE_INTERVAL) {
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
        
        // Format timer string
        unsigned long currentTimer = isTimerRunning ? (currentTime - timerStartTime) : timerElapsedTime;
        unsigned long seconds = (currentTimer / 1000) % 60;
        unsigned long minutes = (currentTimer / 60000) % 100; // Cap at 99 mins
        char timerStr[6];
        snprintf(timerStr, sizeof(timerStr), "%02lu:%02lu", minutes, seconds);

        float flowRate = 0.0f; // Calculate flow rate if needed, for now placeholder
        bool wifiApMode = wifiManager.isApMode();
        bool wifiConnected = wifiManager.isInitialized();
        bool bluetoothConnected = bluetoothScale.isConnected();

        display.showMainScreen(barCount, batteryPercent, weight, timerStr, flowRate, isCharging, animationFrame, batteryDisconnected, wifiConnected, wifiApMode, bluetoothConnected, delayedTarePending);
        
        lastUpdate = currentTime;
    }
    
    if (currentTime - lastAnimationUpdate >= ANIMATION_INTERVAL) {
        animationFrame = (animationFrame + 1) % 4;
        lastAnimationUpdate = currentTime;
    }
}
