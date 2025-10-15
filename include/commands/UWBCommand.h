#ifndef UWB_COMMAND_H
#define UWB_COMMAND_H

#include "interfaces/IExtendedCommand.h"
#include "managers/UWBManager.h"
#include "managers/ConfigManager.h"
static TaskHandle_t xUWBCmdHandle;

class UWBCommand : public IExtendedCommand
{
private:
    UWBManager &uwbManager;
    ConfigManager &configManager;

    bool startUWBCmd(const std::vector<String> &args, ICommandContext &context);
    bool stopUWBCmd(const std::vector<String> &args, ICommandContext &context);
    TaskHandle_t uwbTaskHandle = NULL;

public:
    UWBCommand()
        : uwbManager(UWBManager::getInstance()),
          configManager(ConfigManager::getInstance())
    {
        xUWBCmdHandle = nullptr;
        subCommands["start"] = [this](const std::vector<String> &args, ICommandContext &context)
        {
            return startUWBCmd(args, context);
        };
        subCommandDescriptions["start"] = "Starts the Ultra Wideband module";
        subCommandParameters["start"] = {
            // Todo
        };

        subCommands["stop"] = [this](const std::vector<String> &args, ICommandContext &context)
        {
            return stopUWBCmd(args, context);
        };
        subCommandDescriptions["stop"] = "Stops the Ultra Wideband module";
        subCommandParameters["stop"] = {
            // Todo
        };
    }

    const char *getName() const override
    {
        return "uwb";
    }

    const char *getDescription() const override
    {
        return "Start or stop the UWB-Initator mode";
    }
};

#endif
