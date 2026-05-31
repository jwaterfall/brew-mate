#include "wifi_manager.h"
#include <LittleFS.h>
#include "logger.h"
#include "config_manager.h"
#include "api_handler.h"

WiFiManager::WiFiManager()
    : server(80), apiHandler(nullptr), apMode(false), initialized(false), wifiConnected(false) {}

WiFiManager::~WiFiManager() {
    if (apiHandler) {
        delete apiHandler;
    }
}

bool WiFiManager::connectToWiFi(const String& ssid, const String& password, int timeoutSeconds) {
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
    }

    Logger::warn("WiFi connection failed");
    wifiConnected = false;
    return false;
}

void WiFiManager::setupAP() {
    Logger::info("Setting up WiFi Access Point");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(getAPSSID(), getAPPassword());
    WiFi.softAPConfig(getAPIP(), getAPGateway(), getAPSubnet());
    apMode = true;

    IPAddress ip = WiFi.softAPIP();
    Logger::info("AP started. SSID: %s, IP: %s", getAPSSID(), ip.toString().c_str());
}

void WiFiManager::setupRoutes() {
    apiHandler->setupRoutes();

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // Non-API 404s fall back to index.html for SPA routing.
    server.onNotFound([this](AsyncWebServerRequest *request) {
        if (!request->url().startsWith("/api/")) {
            request->send(LittleFS, "/index.html", "text/html");
        }
    });

    server.begin();
    Logger::info("Web server started");
}

const char* WiFiManager::getAPSSID() { return "BrewMate"; }
const char* WiFiManager::getAPPassword() { return "brewmate123"; }
IPAddress WiFiManager::getAPIP() { return IPAddress(192, 168, 4, 1); }
IPAddress WiFiManager::getAPGateway() { return IPAddress(192, 168, 4, 1); }
IPAddress WiFiManager::getAPSubnet() { return IPAddress(255, 255, 255, 0); }

bool WiFiManager::begin() {
    Logger::info("Initializing WiFi Manager");

    if (!LittleFS.begin(true)) {
        Logger::error("LittleFS initialization failed");
        return false;
    }
    Logger::info("LittleFS initialized");

    DeviceConfig config = ConfigManager::load();

    if (config.wifi.ssid.length() > 0) {
        if (connectToWiFi(config.wifi.ssid, config.wifi.password)) {
            Logger::info("WiFi connected successfully, AP mode disabled");
        } else {
            Logger::warn("WiFi connection failed, starting AP mode");
            setupAP();
        }
    } else {
        Logger::info("No saved WiFi credentials, starting AP mode");
        setupAP();
    }

    apiHandler = new ApiHandler(server);
    apiHandler->setWiFiManager(this);
    setupRoutes();

    initialized = true;
    return true;
}

void WiFiManager::update() {}

void WiFiManager::setBattery(Battery* bat) {
    if (apiHandler) apiHandler->setBattery(bat);
}

void WiFiManager::setDeviceState(DeviceState* state) {
    if (apiHandler) apiHandler->setDeviceState(state);
}

bool WiFiManager::isApMode() const { return apMode; }
bool WiFiManager::isInitialized() const { return initialized; }
bool WiFiManager::isWiFiConnected() const { return wifiConnected; }
String WiFiManager::getConnectedSSID() const { return connectedSSID; }
IPAddress WiFiManager::getConnectedIP() const { return connectedIP; }
AsyncWebServer& WiFiManager::getServer() { return server; }
