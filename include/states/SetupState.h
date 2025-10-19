#ifndef SETUP_STATE_H
#define SETUP_STATE_H

#include <Device.h>
#include <WiFi.h>
#include "managers/LogManager.h"
#include "managers/WifiManager.h"
#include "managers/MQTTManager.h"
#include "managers/BluetoothManager.h"
#include "managers/CommandManager.h"
#include "states/ErrorState.h"
#include "states/ActionState.h"
#include "states/UpdateState.h"
#include "states/TestState.h"
#include "ErrorCodes.h"

enum class SetupPhase
{
    INIT,
    WIFI_CONNECTING,
    MQTT_CONNECTING,
    BLUETOOTH_INIT,
    COMPLETED,
    FAILED,
    __DELIMITER__
};

class SetupState : public IDeviceState
{
private:
    SetupState(Device *device)
        : IDeviceState(device, StateIdentifier::SETUP_STATE), log(LogManager::getInstance()), commandManager(CommandManager::getInstance()), configManager(ConfigManager::getInstance()), wifiManager(WifiManager::getInstance()), mqttManager(MQTTManager::getInstance()), bluetoothManager(BluetoothManager::getInstance()) {};

    LogManager &log;
    ConfigManager &configManager;
    WifiManager &wifiManager;
    MQTTManager &mqttManager;
    CommandManager &commandManager;
    BluetoothManager &bluetoothManager;

    static const uint32_t SETUP_TIMEOUT = 60000;
    SetupPhase currentPhase;
    uint32_t setupStateTime;

    uint8_t mqttConnectionAttempts;

    bool initializeManagers();
    void handleWifiConnection();
    void handleMqttConnection();
    void handleConnectionError(const char *message, ErrorCode errorCode);
    void handleSetupFailure();
    void subscribeDefaultTopics();
    void handleDeviceMessage(const char *topic, const uint8_t *payload, unsigned int length);
    void handleConfigMessage(const char *topic, const uint8_t *payload, unsigned int length);

    const char *getSetupPhaseString(SetupPhase phase);
    constexpr size_t getSetupPhaseCount() { return static_cast<size_t>(SetupPhase::__DELIMITER__); };

public:
    SetupState(const SetupState &) = delete;
    void operator=(const SetupState &) = delete;

    static SetupState &getInstance(Device *device)
    {
        static SetupState instance(device);
        return instance;
    }

    void enter() override;
    void update() override;
    void exit() override;
};

#endif