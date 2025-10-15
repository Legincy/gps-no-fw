#include "commands/UWBCommand.h"

bool UWBCommand::startUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    if (uwbManager.enableInitator())
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

bool UWBCommand::stopUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    if (uwbManager.disableInitator())
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
