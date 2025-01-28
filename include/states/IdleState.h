#ifndef IDLE_STATE_H
#define IDLE_STATE_H

#include <Device.h>

class IdleState : public DeviceState {
private:
    IdleState(Device* device) 
        : DeviceState(device, StateIdentifier::IDLE_STATE)
        , log(Logger::getInstance())
        , configManager(ConfigManager::getInstance()) {};
    
    Logger& log;
    ConfigManager& configManager;

public:
    IdleState(const IdleState&) = delete;
    void operator=(const IdleState&) = delete;

    static IdleState& getInstance(Device* device) {
        static IdleState instance(device);
        return instance;
    }

    void enter() override;
    void update() override;
    void exit() override;
}; 

#endif