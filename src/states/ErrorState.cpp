#include "states/ErrorState.h"

void ErrorState::enter()
{
    log.info("ErrorState", "Entering ErrorState");
    reportError();
    startRecoveryTimer();
}

void ErrorState::update()
{
}

void ErrorState::exit()
{
    log.info("ErrorState", "Exiting ErrorState");
}

void ErrorState::setError(ErrorCode errorCode, IDeviceState *sourceState, const char *message)
{
}

void ErrorState::reportError()
{
    char msgBuffer[256];
    snprintf(msgBuffer, sizeof(msgBuffer), "Error occurred: %s", errorMessage);
    log.error("ErrorState", msgBuffer);

    if (mqttManager.isConnected())
    {
        RuntimeConfig &config = configManager.getRuntimeConfig();
        String topic = config.mqtt.baseTopic + String(config.device.chipID) + "/error";
        mqttManager.publish(topic.c_str(), errorMessage, true); // retain flag = true
    }
}

void ErrorState::startRecoveryTimer()
{
    lastRecoveryAttempt = millis();
}