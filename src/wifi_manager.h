#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <LittleFS.h>
#include "logger.h"

class Battery;
class Scale;

class WiFiManager {
private:
    AsyncWebServer server;
    bool apMode;
    bool initialized;
    Battery* battery;
    Scale* scale;
    
    void setupAP() {
        Logger::info("Setting up WiFi Access Point");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(getAPSSID(), getAPPassword());
        WiFi.softAPConfig(getAPIP(), getAPGateway(), getAPSubnet());
        apMode = true;
        
        IPAddress ip = WiFi.softAPIP();
        Logger::info("AP started. SSID: %s, IP: %s", getAPSSID(), ip.toString().c_str());
    }
    
    void setupRoutes() {
        registerRoute("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
            this->handleStatus(request);
        });
        
        registerRoute("/api/tare", HTTP_POST, [this](AsyncWebServerRequest *request) {
            this->handleTare(request);
        });
        
        server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
        
        server.onNotFound([this](AsyncWebServerRequest *request) {
            if (request->url().startsWith("/api/")) {
                sendJsonError(request, "Not found", 404);
                return;
            }
            request->send(LittleFS, "/index.html", "text/html");
        });
        
        server.begin();
        Logger::info("Web server started");
    }
    
    void registerRoute(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction handler) {
        server.on(uri, method, handler);
    }
    
    void sendJsonResponse(AsyncWebServerRequest *request, std::function<void(JsonObject&)> buildJson, int code = 200) {
        AsyncJsonResponse *response = new AsyncJsonResponse();
        JsonObject root = response->getRoot().to<JsonObject>();
        buildJson(root);
        response->setLength();
        request->send(response);
    }
    
    void sendJsonError(AsyncWebServerRequest *request, const char* error, int code = 400) {
        sendJsonResponse(request, [error](JsonObject& root) {
            root["error"] = error;
        }, code);
    }
    
    void sendJsonSuccess(AsyncWebServerRequest *request, int code = 200) {
        sendJsonResponse(request, [](JsonObject& root) {
            root["success"] = true;
        }, code);
    }
    
    void handleStatus(AsyncWebServerRequest *request) {
        if (!battery || !scale) {
            sendJsonError(request, "Service not initialized", 503);
            return;
        }
        
        sendJsonResponse(request, [this](JsonObject& root) {
            root["weight"] = scale->getWeight();
            root["batteryPercent"] = battery->getPercentage();
            root["batteryVoltage"] = battery->getVoltage();
            root["usbConnected"] = battery->isUsbConnected();
        });
    }
    
    void handleTare(AsyncWebServerRequest *request) {
        if (!scale) {
            sendJsonError(request, "Scale not initialized", 503);
            return;
        }
        
        scale->tare();
        sendJsonSuccess(request);
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
    WiFiManager() : server(80), apMode(false), initialized(false), battery(nullptr), scale(nullptr) {}
    
    bool begin() {
        Logger::info("Initializing WiFi Manager");
        
        if (!LittleFS.begin(true)) {
            Logger::error("LittleFS initialization failed");
            return false;
        }
        Logger::info("LittleFS initialized");
        
        setupAP();
        setupRoutes();
        
        initialized = true;
        return true;
    }
    
    void update() {}
    
    void setBattery(Battery* bat) {
        battery = bat;
    }
    
    void setScale(Scale* scl) {
        scale = scl;
    }
    
    bool isApMode() const {
        return apMode;
    }
    
    bool isInitialized() const {
        return initialized;
    }
    
    AsyncWebServer& getServer() {
        return server;
    }
};

#endif
