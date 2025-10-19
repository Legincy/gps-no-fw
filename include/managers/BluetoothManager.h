#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#define BLE_DEVICE_INFO_SERVICE_UUID "180A"
#define BLE_DEVICE_INFO_CHARACTERISTIC_UUID "2A23"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "LogManager.h"

enum class BluetoothStatus
{
    INACTIVE,
    ACTIVE,
    ADVERTISING,
    ERROR
};

class BluetoothManager
{
private:
    BluetoothManager()
        : status(BluetoothStatus::INACTIVE), pServer(nullptr), pDeviceInfoChar(nullptr), configManager(ConfigManager::getInstance()), log(LogManager::getInstance()) {}

    ConfigManager &configManager;
    LogManager &log;

    BluetoothStatus status;

    BLEServer *pServer;
    BLEService *pDeviceInfoService;
    BLEService *pCustomService;
    BLECharacteristic *pDeviceInfoChar;

    class ServerCallbacks : public BLEServerCallbacks
    {
    private:
        BluetoothManager &manager;
        LogManager &log;

    public:
        ServerCallbacks(BluetoothManager &manager, LogManager &log) : manager(manager), log(log) {}

        void onConnect(BLEServer *pServer) override;
        void onDisconnect(BLEServer *pServer) override;
    };

    class ClientCallbacks : public BLEClientCallbacks
    {
    private:
        BluetoothManager &manager;
        LogManager &log;

    public:
        ClientCallbacks(BluetoothManager &manager) : manager(manager), log(log) {}

        void onConnect(BLEClient *pClient) override;
        void onDisconnect(BLEClient *pClient) override;
    };

    void setupServices();
    void updateDeviceInfo();

public:
    BluetoothManager(const BluetoothManager &) = delete;
    void operator=(const BluetoothManager &) = delete;

    static BluetoothManager &getInstance()
    {
        static BluetoothManager instance;
        return instance;
    }

    bool begin();
    bool startAdvertising();
    void stopAdvertising();
    void shutdown();
    void update();

    BluetoothStatus getStatus() const { return status; }
    const char *getStatusString() const;
};

#endif