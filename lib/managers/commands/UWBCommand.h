#ifndef UWB_COMMAND_H
#define UWB_COMMAND_H

#include "interfaces/IExtendedCommand.h"
#include "UWBManager.h"
#include "ConfigManager.h"
static TaskHandle_t xUWBCmdHandle;

class UWBCommand : public IExtendedCommand
{
private:
    UWBManager &uwbManager;
    ConfigManager &configManager;

    bool startRangeUWBCmd(const std::vector<String> &args, ICommandContext &context);
    bool stopRangeUWBCmd(const std::vector<String> &args, ICommandContext &context);
    bool resetUWBCmd(const std::vector<String> &args, ICommandContext &context);
    bool printUWBStatusCmd(const std::vector<String> &args, ICommandContext &context);
    bool configureUWBCmd(const std::vector<String> &args, ICommandContext &context);
    TaskHandle_t uwbTaskHandle = NULL;

public:
    UWBCommand()
        : uwbManager(UWBManager::getInstance()),
          configManager(ConfigManager::getInstance())
    {
        xUWBCmdHandle = nullptr;
        // start UWB subcommands
        subCommands["start"] = [this](const std::vector<String> &args, ICommandContext &context)
        {
            return startRangeUWBCmd(args, context);
        };
        subCommandDescriptions["start"] = "Start ranging as Ultra Wideband Initator (TAG)";
        subCommandParameters["start"] = {
            {"min_anchors", "Start ranging with mininum anchors", false, ""}};
        // stop UWB subcommands
        subCommands["stop"] = [this](const std::vector<String> &args, ICommandContext &context)
        {
            return stopRangeUWBCmd(args, context);
        };
        subCommandDescriptions["stop"] = "Stop ranging as Ultra Wideband Initator (TAG)";
        // reset UWB subcommands
        subCommands["reset"] = [this](const std::vector<String> &args, ICommandContext &context)
        {
            return resetUWBCmd(args, context);
        };
        subCommandDescriptions["reset"] = "Reset UWB module.";
        // info UWB subcommands
        subCommands["status"] = [this](const std::vector<String> &args, ICommandContext &context)
        {
            return printUWBStatusCmd(args, context);
        };
        subCommandDescriptions["status"] = "Show UWB status.";
        // config UWB subcommands
        subCommands["config"] = [this](const std::vector<String> &args, ICommandContext &context)
        {
            return configureUWBCmd(args, context);
        };
        subCommandDescriptions["config"] = "Configure UWB Module.";
    }

    const char *getName() const override
    {
        return "uwb";
    }

    const char *getDescription() const override
    {
        return "Configure and manage Ultra Wideband (UWB) module";
    }
};

#endif
