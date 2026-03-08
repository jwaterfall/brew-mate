#include "bluetooth_scale.h"
#include "scale.h"

const char* BluetoothScale::SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* BluetoothScale::GAGGIMATE_CHARACTERISTIC_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const char* BluetoothScale::BEAN_CONQUEROR_CHARACTERISTIC_UUID = "6E400004-B5A3-F393-E0A9-E50E24DCCA9E";
const char* BluetoothScale::COMMAND_CHARACTERISTIC_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

BluetoothScale::BluetoothScale()
    : scale(nullptr)
    , server(nullptr)
    , service(nullptr)
    , gaggiMateWeightCharacteristic(nullptr)
    , commandCharacteristic(nullptr)
    , advertising(nullptr)
    , deviceConnected(false)
    , oldDeviceConnected(false)
    , lastHeartbeat(0)
    , lastWeightSent(0)
    , lastWeight(0.0f)
    , connectionRSSI(-100)
    , connectionHandle(0)
{
}

BluetoothScale::~BluetoothScale() {
    end();
}

void BluetoothScale::begin(Scale* scaleInstance) {
    scale = scaleInstance;
    NimBLEDevice::init("WeighMyBru");
    NimBLEDevice::setPower(3);

    server = NimBLEDevice::createServer();
    server->setCallbacks(this);
    server->advertiseOnDisconnect(true);

    service = server->createService(SERVICE_UUID);

    gaggiMateWeightCharacteristic = service->createCharacteristic(
        GAGGIMATE_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::NOTIFY |
        NIMBLE_PROPERTY::INDICATE
    );

    beanConquerorWeightCharacteristic = service->createCharacteristic(
        BEAN_CONQUEROR_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::NOTIFY |
        NIMBLE_PROPERTY::INDICATE
    );

    commandCharacteristic = service->createCharacteristic(
        COMMAND_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::WRITE_NR |
        NIMBLE_PROPERTY::NOTIFY
    );
    commandCharacteristic->setCallbacks(this);

    service->start();

    advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->enableScanResponse(true);
    advertising->setName("WeighMyBru");
    startAdvertising();
}

void BluetoothScale::end() {
    if (server) {
        stopAdvertising();
        NimBLEDevice::deinit();
        server = nullptr;
        service = nullptr;
        gaggiMateWeightCharacteristic = nullptr;
        commandCharacteristic = nullptr;
        advertising = nullptr;
    }
}

void BluetoothScale::initializeBLE() {
}

void BluetoothScale::startAdvertising() {
    if (advertising) {
        advertising->start();
    }
}

void BluetoothScale::stopAdvertising() {
    if (advertising) {
        advertising->stop();
    }
}

void BluetoothScale::update() {
    if (!scale) {
        return;
    }

    uint32_t now = millis();

    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        startAdvertising();
        oldDeviceConnected = deviceConnected;
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        lastHeartbeat = now;
        delay(100);
        sendNotificationRequest();
    }

    if (deviceConnected) {
        if (now - lastWeightSent >= WEIGHT_SEND_INTERVAL) {
            float currentWeight = scale->getWeight();
            sendGaggiMateWeight(currentWeight);
            sendBeanConquerorWeight(currentWeight);
            lastWeight = currentWeight;
            lastWeightSent = now;
        }

        if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
            sendHeartbeat();
            lastHeartbeat = now;
        }
    }
}

bool BluetoothScale::isConnected() const {
    return deviceConnected;
}

int BluetoothScale::getSignalStrength() const {
    if (!deviceConnected || !server) {
        return -100;
    }
    return connectionRSSI;
}

void BluetoothScale::sendGaggiMateWeight(float weight) {
    if (!gaggiMateWeightCharacteristic) {
        return;
    }

    int32_t weightInt = static_cast<int32_t>(weight * 100);
    uint8_t payload[PROTOCOL_LENGTH] = {0};

    payload[0] = PRODUCT_NUMBER;
    payload[1] = static_cast<uint8_t>(WeighMyBruMessageType::WEIGHT);
    payload[6] = (weightInt >= 0) ? 43 : 45;

    uint32_t absWeight = abs(weightInt);
    payload[7] = (absWeight >> 16) & 0xFF;
    payload[8] = (absWeight >> 8) & 0xFF;
    payload[9] = absWeight & 0xFF;

    payload[PROTOCOL_LENGTH - 1] = calculateChecksum(payload, PROTOCOL_LENGTH - 1);

    gaggiMateWeightCharacteristic->setValue(payload, PROTOCOL_LENGTH);
    gaggiMateWeightCharacteristic->notify();
}

void BluetoothScale::sendBeanConquerorWeight(float weight) {
    if (!beanConquerorWeightCharacteristic) {
        return;
    }

    union {
        float floatValue;
        uint8_t bytes[4];
    } weightValue = {.floatValue = weight};

    beanConquerorWeightCharacteristic->setValue(weightValue.bytes, sizeof(weightValue.bytes));
    beanConquerorWeightCharacteristic->notify();
}

void BluetoothScale::sendHeartbeat() {
    if (!deviceConnected) {
        return;
    }
    uint8_t payload[] = {0x02, 0x00};
    sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
}

void BluetoothScale::sendNotificationRequest() {
    if (!deviceConnected) {
        return;
    }
    uint8_t payload[] = {0x06, 0x00, 0x00, 0x00, 0x00, 0x00};
    sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
}

void BluetoothScale::sendMessage(WeighMyBruMessageType msgType, const uint8_t* payload, size_t length) {
    if (!deviceConnected || !commandCharacteristic) {
        return;
    }

    uint8_t* message = new uint8_t[length + 1];
    memcpy(message, payload, length);
    message[length] = calculateChecksum(message, length);

    commandCharacteristic->setValue(message, length + 1);
    delete[] message;
}

uint8_t BluetoothScale::calculateChecksum(const uint8_t* data, size_t length) const {
    uint8_t checksum = data[0];
    for (size_t i = 1; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

void BluetoothScale::handleTareCommand() {
    if (tareCallback) {
        tareCallback();
    } else if (scale) {
        scale->tare();
    }

    uint8_t payload[] = {0x03, 0x0a, 0x01, 0x00, 0x00};
    sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
}

void BluetoothScale::processIncomingMessage(const uint8_t* data, size_t length) {
    if (length < 2) {
        return;
    }

    uint8_t productNumber = data[0];
    WeighMyBruMessageType messageType = static_cast<WeighMyBruMessageType>(data[1]);

    if (productNumber != PRODUCT_NUMBER) {
        return;
    }

    if (messageType == WeighMyBruMessageType::SYSTEM && length >= 4) {
        if (data[2] == 0x01 && data[3] == 0x01) {
            handleTareCommand();
        }
    }
}

void BluetoothScale::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    deviceConnected = true;
}

void BluetoothScale::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    deviceConnected = false;
}

void BluetoothScale::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
    std::string value = pCharacteristic->getValue();
    if (!value.empty()) {
        processIncomingMessage(reinterpret_cast<const uint8_t*>(value.data()), value.length());
    }
}

