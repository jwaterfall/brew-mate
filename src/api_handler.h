#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <Arduino.h>
#include <functional>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "config_manager.h"
#include "logger.h"

class Battery;
class Scale;
class WiFiManager;

class ApiHandler {
private:
    AsyncWebServer& server;
    Battery* battery;
    Scale* scale;
    WiFiManager* wifiManager;
    
    void handleCors(AsyncWebServerRequest *request);
    void addCorsHeaders(AsyncWebServerResponse *response);
    void sendJsonResponse(AsyncWebServerRequest *request, std::function<void(JsonObject&)> buildJson, int code = 200);
    void sendJsonError(AsyncWebServerRequest *request, const char* error, int code = 400);
    void sendJsonSuccess(AsyncWebServerRequest *request, int code = 200);
    void registerRoute(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction handler);
    
    void handleStatus(AsyncWebServerRequest *request);
    void handleTare(AsyncWebServerRequest *request);
    void handleWiFiStatus(AsyncWebServerRequest *request);
    void handleGetWiFiConfig(AsyncWebServerRequest *request);
    void handlePostWiFiConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    void handleGetBatteryCalibration(AsyncWebServerRequest *request);
    void handlePostBatteryCalibration(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    
public:
    ApiHandler(AsyncWebServer& server);
    
    void setupRoutes();
    
    void setBattery(Battery* bat);
    void setScale(Scale* scl);
    void setWiFiManager(WiFiManager* wifi);
};

#endif

