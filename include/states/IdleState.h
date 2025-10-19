#ifndef IDLE_STATE_H
#define IDLE_STATE_H

#include <Device.h>
#include "states/SetupState.h"

class IdleState : public IDeviceState
{
private:
    IdleState(Device *device)
        : IDeviceState(device, StateIdentifier::IDLE_STATE), log(LogManager::getInstance()), configManager(ConfigManager::getInstance()) {};

    LogManager &log;
    ConfigManager &configManager;

public:
    IdleState(const IdleState &) = delete;
    void operator=(const IdleState &) = delete;

    static IdleState &getInstance(Device *device)
    {
        static IdleState instance(device);
        return instance;
    }

    void enter() override;
    void update() override;
    void exit() override;
};

#endif