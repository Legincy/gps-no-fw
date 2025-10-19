#ifndef DEVICE_H
#define DEVICE_H

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 1
#define VERSION_STRING STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)

#define JSON_DOC_SIZE 512

#include <memory>
#include <ArduinoJson.h>
#include "managers/ConfigManager.h"
#include "managers/MQTTManager.h"
#include "managers/LogManager.h"
#include "managers/CommandManager.h"
#include "interfaces/IDeviceState.h"
#include "contexts/SerialCommandContext.h"
#include "contexts/MQTTCommandContext.h"

enum class DeviceStatus
{
    BOOTING,
    IDLE,
    SETUP,
    TRANSITIONING,
    ACTION,
    ERROR,
    __DELIMITER__
};

class IDeviceState;

class Device
{
private:
    Device()
        : currentState(nullptr), lastStatusUpdate(0), mqttManager(MQTTManager::getInstance()), configManager(ConfigManager::getInstance()), log(LogManager::getInstance()), commandManager(CommandManager::getInstance()) {}

    MQTTManager &mqttManager;
    ConfigManager &configManager;
    CommandManager &commandManager;
    LogManager &log;

    IDeviceState *currentState;
    uint32_t lastStatusUpdate;

    SerialCommandContext serialContext;
    std::unique_ptr<MQTTCommandContext> mqttContext;

    void sendDeviceStatus();
    void updateDeviceStatus();

    const char *getDeviceStatusString(DeviceStatus status);
    constexpr size_t getDeviceStatusCount() { return static_cast<size_t>(DeviceStatus::__DELIMITER__); };

public:
    Device(const Device &) = delete;
    void operator=(const Device &) = delete;

    static Device &getInstance()
    {
        static Device instance;
        return instance;
    }

    bool begin();
    void changeState(IDeviceState &newState);
    void update();
    IDeviceState *getCurrentState() { return currentState; }
};

#endif