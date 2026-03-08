#include "api_handler.h"
#include "wifi_manager.h"
#include "battery.h"
#include "scale.h"

ApiHandler::ApiHandler(AsyncWebServer& server) 
    : server(server), battery(nullptr), scale(nullptr), wifiManager(nullptr) {}

void ApiHandler::handleCors(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(204);
    addCorsHeaders(response);
    request->send(response);
}

void ApiHandler::addCorsHeaders(AsyncWebServerResponse *response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void ApiHandler::sendJsonResponse(AsyncWebServerRequest *request, std::function<void(JsonObject&)> buildJson, int code) {
    AsyncJsonResponse *response = new AsyncJsonResponse(false);
    JsonObject root = response->getRoot().to<JsonObject>();
    buildJson(root);
    addCorsHeaders(response);
    response->setLength();
    AsyncWebServerResponse *baseResponse = (AsyncWebServerResponse*)response;
    baseResponse->setCode(code);
    request->send(response);
}

void ApiHandler::sendJsonError(AsyncWebServerRequest *request, const char* error, int code) {
    sendJsonResponse(request, [error](JsonObject& root) {
        root["error"] = error;
    }, code);
}

void ApiHandler::sendJsonSuccess(AsyncWebServerRequest *request, int code) {
    sendJsonResponse(request, [](JsonObject& root) {
        root["success"] = true;
    }, code);
}

void ApiHandler::registerRoute(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction handler) {
    server.on(uri, method, handler);
}

void ApiHandler::handleStatus(AsyncWebServerRequest *request) {
    if (!battery || !scale || !wifiManager) {
        sendJsonError(request, "Service not initialized", 503);
        return;
    }
    
    sendJsonResponse(request, [this](JsonObject& root) {
        root["weight"] = scale->getWeight();
        root["batteryPercent"] = battery->getPercentage();
        root["batteryVoltage"] = battery->getVoltage();
        root["usbConnected"] = battery->isUsbConnected();
        root["batteryDisconnected"] = battery->isBatteryDisconnected();
        root["wifiConnected"] = wifiManager->isWiFiConnected();
        root["wifiApMode"] = wifiManager->isApMode();
        if (wifiManager->isWiFiConnected()) {
            root["wifiSSID"] = wifiManager->getConnectedSSID();
            root["wifiIP"] = wifiManager->getConnectedIP().toString();
        }
    });
}

void ApiHandler::handleTare(AsyncWebServerRequest *request) {
    if (!scale) {
        sendJsonError(request, "Scale not initialized", 503);
        return;
    }
    
    scale->tare();
    sendJsonSuccess(request);
}

void ApiHandler::handleWiFiStatus(AsyncWebServerRequest *request) {
    sendJsonResponse(request, [this](JsonObject& root) {
        root["connected"] = wifiManager->isWiFiConnected();
        root["apMode"] = wifiManager->isApMode();
        if (wifiManager->isWiFiConnected()) {
            root["ssid"] = wifiManager->getConnectedSSID();
            root["ip"] = wifiManager->getConnectedIP().toString();
        }
    });
}

void ApiHandler::handleGetWiFiConfig(AsyncWebServerRequest *request) {
    WiFiConfig wifiConfig = ConfigManager::getWiFiConfig();
    sendJsonResponse(request, [wifiConfig](JsonObject& root) {
        root["ssid"] = wifiConfig.ssid;
    });
}

void ApiHandler::handlePostWiFiConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, (char*)data);
    
    if (error) {
        sendJsonError(request, "Invalid JSON", 400);
        return;
    }
    
    if (!doc["ssid"].is<String>() || !doc["password"].is<String>()) {
        sendJsonError(request, "Missing ssid or password", 400);
        return;
    }
    
    WiFiConfig wifiConfig;
    wifiConfig.ssid = doc["ssid"].as<String>();
    wifiConfig.password = doc["password"].as<String>();
    
    if (wifiConfig.ssid.length() == 0) {
        sendJsonError(request, "SSID cannot be empty", 400);
        return;
    }
    
    if (ConfigManager::saveWiFiConfig(wifiConfig)) {
        sendJsonSuccess(request);
        Logger::info("WiFi config saved via API. Will connect on next restart.");
    } else {
        sendJsonError(request, "Failed to save config", 500);
    }
}

void ApiHandler::handleGetBatteryCalibration(AsyncWebServerRequest *request) {
    if (!battery) {
        sendJsonError(request, "Battery not initialized", 503);
        return;
    }
    
    sendJsonResponse(request, [this](JsonObject& root) {
        root["batteryCalibrationFactor"] = battery->getCalibrationFactor();
        root["vbusCalibrationFactor"] = battery->getVbusCalibrationFactor();
    });
}

void ApiHandler::handlePostBatteryCalibration(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    if (!battery) {
        sendJsonError(request, "Battery not initialized", 503);
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, (char*)data);
    
    if (error) {
        sendJsonError(request, "Invalid JSON", 400);
        return;
    }
    
    BatteryConfig batteryConfig = ConfigManager::getBatteryConfig();
    
    if (doc["batteryCalibrationFactor"].is<float>()) {
        batteryConfig.batteryCalibrationFactor = doc["batteryCalibrationFactor"].as<float>();
    }
    if (doc["vbusCalibrationFactor"].is<float>()) {
        batteryConfig.vbusCalibrationFactor = doc["vbusCalibrationFactor"].as<float>();
    }
    
    battery->setCalibrationFactor(batteryConfig.batteryCalibrationFactor);
    battery->setVbusCalibrationFactor(batteryConfig.vbusCalibrationFactor);
    
    if (ConfigManager::saveBatteryConfig(batteryConfig)) {
        sendJsonSuccess(request);
        Logger::info("Battery calibration saved via API");
    } else {
        sendJsonError(request, "Failed to save calibration", 500);
    }
}

void ApiHandler::setupRoutes() {
    // CORS preflight handlers
    server.on("/api/status", HTTP_OPTIONS, [this](AsyncWebServerRequest *request) {
        this->handleCors(request);
    });
    server.on("/api/tare", HTTP_OPTIONS, [this](AsyncWebServerRequest *request) {
        this->handleCors(request);
    });
    server.on("/api/wifi/status", HTTP_OPTIONS, [this](AsyncWebServerRequest *request) {
        this->handleCors(request);
    });
    server.on("/api/wifi/config", HTTP_OPTIONS, [this](AsyncWebServerRequest *request) {
        this->handleCors(request);
    });
    server.on("/api/battery/calibration", HTTP_OPTIONS, [this](AsyncWebServerRequest *request) {
        this->handleCors(request);
    });
    
    // API routes
    registerRoute("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleStatus(request);
    });
    
    registerRoute("/api/tare", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleTare(request);
    });
    
    registerRoute("/api/wifi/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleWiFiStatus(request);
    });
    
    registerRoute("/api/wifi/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleGetWiFiConfig(request);
    });
    
    server.on("/api/wifi/config", HTTP_POST, [this](AsyncWebServerRequest *request) {}, nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            this->handlePostWiFiConfig(request, data, len);
        });
    
    registerRoute("/api/battery/calibration", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleGetBatteryCalibration(request);
    });
    
    server.on("/api/battery/calibration", HTTP_POST, [this](AsyncWebServerRequest *request) {}, nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            this->handlePostBatteryCalibration(request, data, len);
        });
    
    // 404 handler for API routes
    server.onNotFound([this](AsyncWebServerRequest *request) {
        if (request->url().startsWith("/api/")) {
            sendJsonError(request, "Not found", 404);
            return;
        }
        // Let WiFiManager handle static files
    });
}

void ApiHandler::setBattery(Battery* bat) { battery = bat; }
void ApiHandler::setScale(Scale* scl) { scale = scl; }
void ApiHandler::setWiFiManager(WiFiManager* wifi) { wifiManager = wifi; }
