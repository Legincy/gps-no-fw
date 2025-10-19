#include "managers/BluetoothManager.h"

bool BluetoothManager::begin()
{
    if (status != BluetoothStatus::INACTIVE)
    {
        log.error("BluetoothManager", "BluetoothManager is already active.");
        return false;
    }

    const RuntimeConfig &config = configManager.getRuntimeConfig();

    try
    {
        BLEDevice::init(config.device.name);
        pServer = BLEDevice::createServer();

        if (!pServer)
        {
            log.error("BluetoothManager", "Failed to create BLE server.");
            status = BluetoothStatus::ERROR;

            return false;
        }

        pServer->setCallbacks(new ServerCallbacks(*this, log));

        setupServices();

        status = BluetoothStatus::ACTIVE;
        log.info("BluetoothManager", "Successfully initialized BluetoothManager.");

        return true;
    }
    catch (const std::exception &e)
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Error starting BluetoothManager: %s", e.what());
        log.error("BluetoothManager", buffer);

        return false;
    }
}

void BluetoothManager::updateDeviceInfo()
{
    return;
}

void BluetoothManager::setupServices()
{
    const RuntimeConfig &config = configManager.getRuntimeConfig();

    pDeviceInfoService = pServer->createService("180A");
    pDeviceInfoChar = pDeviceInfoService->createCharacteristic("2A24", BLECharacteristic::PROPERTY_READ);
    pCustomService = pServer->createService(config.bluetooth.serviceUUID);

    pDeviceInfoService->start();
    pCustomService->start();

    updateDeviceInfo();
}

const char *BluetoothManager::getStatusString() const
{
    switch (status)
    {
    case BluetoothStatus::INACTIVE:
        return "INACTIVE";
    case BluetoothStatus::ACTIVE:
        return "ACTIVE";
    case BluetoothStatus::ADVERTISING:
        return "ADVERTISING";
    case BluetoothStatus::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void BluetoothManager::update()
{
    // TODO:
    return;
}

void BluetoothManager::ServerCallbacks::onConnect(BLEServer *pServer)
{
    return;
}

void BluetoothManager::ServerCallbacks::onDisconnect(BLEServer *pServer)
{
    return;
}