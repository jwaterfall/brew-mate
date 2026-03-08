#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "logger.h"

struct WiFiConfig {
    String ssid;
    String password;
};

struct BatteryConfig {
    float batteryCalibrationFactor;
    float vbusCalibrationFactor;
};

struct DeviceConfig {
    WiFiConfig wifi;
    BatteryConfig battery;
};

class ConfigManager {
private:
    static const char* CONFIG_FILE;
    
    static bool loadConfig(DeviceConfig& config) {
        if (!LittleFS.exists(CONFIG_FILE)) {
            Logger::info("No config file found, using defaults");
            return false;
        }
        
        File file = LittleFS.open(CONFIG_FILE, "r");
        if (!file) {
            Logger::error("Failed to open config file");
            return false;
        }
        
        size_t size = file.size();
        if (size > 2048) {
            Logger::error("Config file too large");
            file.close();
            return false;
        }
        
        String content = file.readString();
        file.close();
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, content);
        
        if (error) {
            Logger::error("Failed to parse config: %s", error.c_str());
            return false;
        }
        
        // Load WiFi config
        if (doc["wifi"].is<JsonObject>()) {
            JsonObject wifiObj = doc["wifi"];
            if (wifiObj["ssid"].is<String>()) {
                config.wifi.ssid = wifiObj["ssid"].as<String>();
            }
            if (wifiObj["password"].is<String>()) {
                config.wifi.password = wifiObj["password"].as<String>();
            }
        }
        
        // Load Battery config
        if (doc["battery"].is<JsonObject>()) {
            JsonObject batteryObj = doc["battery"];
            if (batteryObj["batteryCalibrationFactor"].is<float>()) {
                config.battery.batteryCalibrationFactor = batteryObj["batteryCalibrationFactor"].as<float>();
            }
            if (batteryObj["vbusCalibrationFactor"].is<float>()) {
                config.battery.vbusCalibrationFactor = batteryObj["vbusCalibrationFactor"].as<float>();
            }
        }
        
        Logger::info("Loaded config from file");
        return true;
    }
    
public:
    static DeviceConfig getDefaultConfig() {
        DeviceConfig config;
        config.wifi.ssid = "";
        config.wifi.password = "";
        config.battery.batteryCalibrationFactor = 1.0f;
        config.battery.vbusCalibrationFactor = 1.0f;
        return config;
    }
    
    static DeviceConfig load() {
        DeviceConfig config = getDefaultConfig();
        loadConfig(config);
        return config;
    }
    
    static bool save(const DeviceConfig& config) {
        File file = LittleFS.open(CONFIG_FILE, "w");
        if (!file) {
            Logger::error("Failed to create config file");
            return false;
        }
        
        JsonDocument doc;
        
        // Save WiFi config
        JsonObject wifiObj = doc["wifi"].to<JsonObject>();
        wifiObj["ssid"] = config.wifi.ssid;
        wifiObj["password"] = config.wifi.password;
        
        // Save Battery config
        JsonObject batteryObj = doc["battery"].to<JsonObject>();
        batteryObj["batteryCalibrationFactor"] = config.battery.batteryCalibrationFactor;
        batteryObj["vbusCalibrationFactor"] = config.battery.vbusCalibrationFactor;
        
        if (serializeJson(doc, file) == 0) {
            Logger::error("Failed to write config");
            file.close();
            return false;
        }
        
        file.close();
        Logger::info("Saved config to file");
        return true;
    }
    
    static bool reset() {
        if (LittleFS.exists(CONFIG_FILE)) {
            return LittleFS.remove(CONFIG_FILE);
        }
        return true;
    }
    
    static WiFiConfig getWiFiConfig() {
        DeviceConfig config = load();
        return config.wifi;
    }
    
    static bool saveWiFiConfig(const WiFiConfig& wifiConfig) {
        DeviceConfig config = load();
        config.wifi = wifiConfig;
        return save(config);
    }
    
    static BatteryConfig getBatteryConfig() {
        DeviceConfig config = load();
        return config.battery;
    }
    
    static bool saveBatteryConfig(const BatteryConfig& batteryConfig) {
        DeviceConfig config = load();
        config.battery = batteryConfig;
        return save(config);
    }
};

inline const char* ConfigManager::CONFIG_FILE = "/device_config.json";

#endif

