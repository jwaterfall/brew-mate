#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "logger.h"
#include "config_manager.h"
#include "api_handler.h"

class WiFiManager {
private:
    AsyncWebServer server;
    ApiHandler* apiHandler;
    bool apMode;
    bool initialized;
    bool wifiConnected;
    String connectedSSID;
    IPAddress connectedIP;
    
    bool connectToWiFi(const String& ssid, const String& password, int timeoutSeconds = 15) {
        Logger::info("Connecting to WiFi: %s", ssid.c_str());
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        
        unsigned long startTime = millis();

        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < (timeoutSeconds * 1000)) {
            delay(500);
            Serial.print(".");
        }

        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            connectedSSID = ssid;
            connectedIP = WiFi.localIP();
            Logger::info("WiFi connected! IP: %s", connectedIP.toString().c_str());
            return true;
        } else {
            Logger::warn("WiFi connection failed");
            wifiConnected = false;
            return false;
        }
    }
    
    void setupAP() {
        Logger::info("Setting up WiFi Access Point");
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(getAPSSID(), getAPPassword());
        WiFi.softAPConfig(getAPIP(), getAPGateway(), getAPSubnet());
        apMode = true;
        
        IPAddress ip = WiFi.softAPIP();
        Logger::info("AP started. SSID: %s, IP: %s", getAPSSID(), ip.toString().c_str());
    }
    
    void setupRoutes() {
        // Setup API routes
        apiHandler->setupRoutes();
        
        // Setup static file serving
        server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
        
        // Handle non-API 404s (fallback to index.html for SPA routing)
        server.onNotFound([this](AsyncWebServerRequest *request) {
            if (!request->url().startsWith("/api/")) {
                request->send(LittleFS, "/index.html", "text/html");
            }
        });
        
        server.begin();
        Logger::info("Web server started");
    }
    
    static const char* getAPSSID() {
        return "BrewMate";
    }
    
    static const char* getAPPassword() {
        return "brewmate123";
    }
    
    static IPAddress getAPIP() {
        return IPAddress(192, 168, 4, 1);
    }
    
    static IPAddress getAPGateway() {
        return IPAddress(192, 168, 4, 1);
    }
    
    static IPAddress getAPSubnet() {
        return IPAddress(255, 255, 255, 0);
    }
    
public:
    WiFiManager() : server(80), apiHandler(nullptr), apMode(false), initialized(false), wifiConnected(false) {}
    
    ~WiFiManager() {
        if (apiHandler) {
            delete apiHandler;
        }
    }
    
    bool begin() {
        Logger::info("Initializing WiFi Manager");
        
        if (!LittleFS.begin(true)) {
            Logger::error("LittleFS initialization failed");
            return false;
        }
        Logger::info("LittleFS initialized");
        
        // Load config
        DeviceConfig config = ConfigManager::load();
        
        // Try to load and connect to saved WiFi credentials
        if (config.wifi.ssid.length() > 0) {
            if (connectToWiFi(config.wifi.ssid, config.wifi.password)) {
                // Connected successfully, no AP mode needed
                Logger::info("WiFi connected successfully, AP mode disabled");
            } else {
                // Connection failed, fall back to AP mode
                Logger::warn("WiFi connection failed, starting AP mode");
                setupAP();
            }
        } else {
            // No saved credentials, start AP mode
            Logger::info("No saved WiFi credentials, starting AP mode");
            setupAP();
        }
        
        // Create and setup API handler
        apiHandler = new ApiHandler(server);
        apiHandler->setWiFiManager(this);
        setupRoutes();
        
        initialized = true;
        return true;
    }
    
    void update() {}
    
    void setBattery(Battery* bat) {
        if (apiHandler) {
            apiHandler->setBattery(bat);
        }
    }
    
    void setScale(Scale* scl) {
        if (apiHandler) {
            apiHandler->setScale(scl);
        }
    }
    
    bool isApMode() const {
        return apMode;
    }
    
    bool isInitialized() const {
        return initialized;
    }
    
    bool isWiFiConnected() const {
        return wifiConnected;
    }
    
    String getConnectedSSID() const {
        return connectedSSID;
    }
    
    IPAddress getConnectedIP() const {
        return connectedIP;
    }
    
    AsyncWebServer& getServer() {
        return server;
    }
};

#endif
