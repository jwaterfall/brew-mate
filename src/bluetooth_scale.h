#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <functional>

class Scale;

using BluetoothEventHandler = std::function<void()>;

enum class WeighMyBruMessageType : uint8_t {
    SYSTEM = 0x0A,
    WEIGHT = 0x0B
};

class BluetoothScale : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
public:
    BluetoothScale();
    ~BluetoothScale();

    BluetoothScale(const BluetoothScale&) = delete;
    BluetoothScale& operator=(const BluetoothScale&) = delete;
    BluetoothScale(BluetoothScale&&) = delete;
    BluetoothScale& operator=(BluetoothScale&&) = delete;

    void begin(Scale* scaleInstance);
    void end();
    void update();
    bool isConnected() const;
    int getSignalStrength() const;

    void onTare(BluetoothEventHandler cb) { tareCallback = cb; }

    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;

private:
    static const char* SERVICE_UUID;
    static const char* GAGGIMATE_CHARACTERISTIC_UUID;
    static const char* BEAN_CONQUEROR_CHARACTERISTIC_UUID;
    static const char* COMMAND_CHARACTERISTIC_UUID;

    static constexpr uint8_t PRODUCT_NUMBER = 0x03;
    static constexpr size_t PROTOCOL_LENGTH = 20;
    static constexpr uint32_t WEIGHT_SEND_INTERVAL = 50;
    static constexpr uint32_t HEARTBEAT_INTERVAL = 2000;

    Scale* scale;
    NimBLEServer* server;
    NimBLEService* service;
    NimBLECharacteristic* gaggiMateWeightCharacteristic;
    NimBLECharacteristic* beanConquerorWeightCharacteristic;
    NimBLECharacteristic* commandCharacteristic;
    NimBLEAdvertising* advertising;

    bool deviceConnected;
    bool oldDeviceConnected;
    uint32_t lastHeartbeat;
    uint32_t lastWeightSent;
    float lastWeight;
    int connectionRSSI;
    uint16_t connectionHandle;

    BluetoothEventHandler tareCallback;

    void initializeBLE();
    void startAdvertising();
    void stopAdvertising();

    void sendGaggiMateWeight(float weight);
    void sendBeanConquerorWeight(float weight);
    void sendHeartbeat();
    void sendNotificationRequest();
    void sendMessage(WeighMyBruMessageType msgType, const uint8_t* payload, size_t length);
    uint8_t calculateChecksum(const uint8_t* data, size_t length) const;

    void handleTareCommand();
    void processIncomingMessage(const uint8_t* data, size_t length);
};
