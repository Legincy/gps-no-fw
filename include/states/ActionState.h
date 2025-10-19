#ifndef ACTION_SATE_H
#define ACTION_SATE_H

#include "Device.h"

class ActionState : public IDeviceState
{
private:
    ActionState(Device *device)
        : IDeviceState(device, StateIdentifier::ACTION_STATE), log(LogManager::getInstance()), configManager(ConfigManager::getInstance()) {};

    LogManager &log;
    ConfigManager &configManager;

public:
    ActionState(const ActionState &) = delete;
    void operator=(const ActionState &) = delete;

    static ActionState &getInstance(Device *device)
    {
        static ActionState instance(device);
        return instance;
    }

    void enter() override;
    void update() override;
    void exit() override;
};

#endif