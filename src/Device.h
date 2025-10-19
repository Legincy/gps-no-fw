#ifndef DEVICE_H
#define DEVICE_H

#include "ConfigManager.h"
#include "MQTTManager.h"
#include "LogManager.h"
#include "WifiManager.h"
#include "UWBManager.h"
#include "CommandManager.h"

class IDeviceState;

class Device
{
private:
    Device()
        : mqttManager(MQTTManager::getInstance()), configManager(ConfigManager::getInstance()),
          log(LogManager::getInstance()), wifiManager(WifiManager::getInstance()),
          uwbManager(UWBManager::getInstance()), commandManager(CommandManager::getInstance()) {}
    WifiManager &wifiManager;
    MQTTManager &mqttManager;
    ConfigManager &configManager;
    LogManager &log;
    UWBManager &uwbManager;
    CommandManager &commandManager;

public:
    Device(const Device &) = delete;

    static Device &getInstance()
    {
        static Device instance;
        return instance;
    }
    bool begin();
    void update();
};

#endif