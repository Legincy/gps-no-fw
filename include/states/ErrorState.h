#ifndef ERROR_STATE_H
#define ERROR_STATE_H

#include <Device.h>
#include <ErrorCodes.h>

class ErrorState : public DeviceState {
private:
    ErrorState(Device* device) 
        : DeviceState(device, StateIdentifier::ERROR_STATE)
        , errorCode(ErrorCode::UNKNOWN_ERROR)
        , log(LogManager::getInstance())
        , mqttManager(MQTTManager::getInstance())
        , configManager(ConfigManager::getInstance()) {};
    
    LogManager& log;
    ConfigManager& configManager;
    MQTTManager& mqttManager;

    ErrorCode errorCode;
    DeviceState* sourceState;
    const char* errorMessage;
    uint8_t recoveryAttempts;
    unsigned long lastRecoveryAttempt;

    bool attemptRecovery();
    void reportError();
    void startRecoveryTimer();
    bool shouldAttemptRecovery() const;
    DeviceState* determineNextState();

    constexpr size_t getErrorCodeCount() {return static_cast<size_t>(ErrorCode::__DELIMITER__);};

public:
    ErrorState(const ErrorState&) = delete;
    void operator=(const ErrorState&) = delete;

    static ErrorState& getInstance(Device* device) {
        static ErrorState instance(device);
        return instance;
    }

    void setError(ErrorCode errorCode, DeviceState* sourceState, const char* message);
    const char* getErrorMessage() const { return errorMessage; }
    ErrorCode getErrorCode() const { return errorCode; }

    void enter() override;
    void update() override;
    void exit() override;
};

#endif  