#include "commands/UWBCommand.h"

bool UWBCommand::startUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    if (configManager.isDeviceTag())
    {
        context.sendResponse("Device is already configured as Initator.\n");
        return false;
    }
    else
    {
        if (uwbManager.enableInitiator())
        {
            context.sendResponse("UWB-Initator mode started.\n");
            return true;
        }
        else
        {
            context.sendResponse("Failed to start UWB-Initator mode.\n");
            return false;
        }
    }
}

bool UWBCommand::stopUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    if (configManager.isDeviceTag())
    {
        if (uwbManager.disableInitiator())
        {
            context.sendResponse("UWB-Initator mode stopped.\n");
            return true;
        }
        else
        {
            context.sendResponse("Failed to stop UWB-Initator mode.\n");
            return false;
        }
    }
    else
    {
        context.sendResponse("Device is not configured as Initator.\n");
        return false;
    }
}
