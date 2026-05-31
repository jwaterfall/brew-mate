#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

class ApiHandler;
class Battery;
struct DeviceState;

// Owns the AsyncWebServer. Connects to saved STA credentials or falls back to
// the BrewMate AP, serves the SPA from LittleFS, and delegates /api/* to
// ApiHandler.
class WiFiManager {
private:
    AsyncWebServer server;
    ApiHandler* apiHandler;
    bool apMode;
    bool initialized;
    bool wifiConnected;
    String connectedSSID;
    IPAddress connectedIP;

    bool connectToWiFi(const String& ssid, const String& password, int timeoutSeconds = 15);
    void setupAP();
    void setupRoutes();

    static const char* getAPSSID();
    static const char* getAPPassword();
    static IPAddress getAPIP();
    static IPAddress getAPGateway();
    static IPAddress getAPSubnet();

public:
    WiFiManager();
    ~WiFiManager();

    bool begin();
    void update();

    void setBattery(Battery* bat);
    void setDeviceState(DeviceState* state);

    bool isApMode() const;
    bool isInitialized() const;
    bool isWiFiConnected() const;
    String getConnectedSSID() const;
    IPAddress getConnectedIP() const;
    AsyncWebServer& getServer();
};
