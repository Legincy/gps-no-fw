#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

class Device;

enum class StateIdentifier {
    ACTION_STATE,
    IDLE_STATE,
    SETUP_STATE,
    ERROR_STATE,
    UPDATE_STATE,
    __DELIMITER__
};

class DeviceState {
protected:
    DeviceState(Device* device, StateIdentifier stateIdentifier) 
        : device(device)
        , stateIdentifier(stateIdentifier) {};

    Device* device;
    StateIdentifier stateIdentifier;

public:
    DeviceState(const DeviceState&) = delete;
    void operator=(const DeviceState&) = delete;
    virtual ~DeviceState() = default;    
    
    virtual void enter() = 0;
    virtual void exit() = 0;
    virtual void update() = 0;

    StateIdentifier getStateIdentifier() const { return stateIdentifier; };
    constexpr size_t getStateIdentifierCount() { return static_cast<size_t>(StateIdentifier::__DELIMITER__); };
    const char* getStateIdentifierString() const {
        switch (stateIdentifier) {
            case StateIdentifier::IDLE_STATE: return "IDLE_STATE";
            case StateIdentifier::SETUP_STATE: return "SETUP_STATE";
            case StateIdentifier::ERROR_STATE: return "ERROR_STATE";
            case StateIdentifier::ACTION_STATE: return "ACTION_STATE";
            case StateIdentifier::UPDATE_STATE: return "UPDATE_STATE";
            default: return "UNKNOWN";
        }
    };
    
};

#endif