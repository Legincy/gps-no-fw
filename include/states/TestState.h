#ifndef TEST_STATE_H
#define TEST_STATE_H

#include <Device.h>

class TestState : public IDeviceState
{
private:
    TestState(Device *device)
        : IDeviceState(device, StateIdentifier::TEST_STATE), log(LogManager::getInstance()), configManager(ConfigManager::getInstance()) {};

    LogManager &log;
    ConfigManager &configManager;

public:
    TestState(const TestState &) = delete;
    void operator=(const TestState &) = delete;

    static TestState &getInstance(Device *device)
    {
        static TestState instance(device);
        return instance;
    }

    void enter() override;
    void update() override;
    void exit() override;
};

#endif