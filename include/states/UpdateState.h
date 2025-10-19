#ifndef UPDATE_STATE_H
#define UPDATE_STATE_H

#include <Update.h>
#include <HTTPClient.h>
#include "ActionState.h"
#include "states/ErrorState.h"
#include "interfaces/IDeviceState.h"
#include "Device.h"
#include "ErrorCodes.h"

enum class UpdatePhase
{
    CHECK_VERSION,
    DOWNLOAD,
    INSTALL,
    COMPLETED,
    FAILED,
    __DELIMITER__
};

class UpdateState : public IDeviceState
{
private:
    UpdateState(Device *device)
        : IDeviceState(device, StateIdentifier::UPDATE_STATE), log(LogManager::getInstance()), configManager(ConfigManager::getInstance()), mqttManager(MQTTManager::getInstance()), currentPhase(UpdatePhase::CHECK_VERSION), lastUpdateCheck(0) {};

    LogManager &log;
    ConfigManager &configManager;
    MQTTManager &mqttManager;

    UpdatePhase currentPhase;
    String newVersion;
    String downloadUrl;
    size_t totalBytes;
    size_t downloadedBytes;
    uint32_t lastUpdateCheck;

    bool checkLatestRelease();
    bool downloadAndInstall();
    void reportProgress(const char *status, int progress = -1);
    void handleUpdateError(const char *message);
    bool compareVersion(const char *current, const char *newer);
    bool checkUpdateConditions();

public:
    UpdateState(const UpdateState &) = delete;
    void operator=(const UpdateState &) = delete;

    static UpdateState &getInstance(Device *device)
    {
        static UpdateState instance(device);
        return instance;
    }

    void enter() override;
    void update() override;
    void exit() override;
};

#endif