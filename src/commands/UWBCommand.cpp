#include "commands/UWBCommand.h"

bool UWBCommand::startRangeUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    if (configManager.isDeviceTag())
    {
        context.sendResponse("Ranging mode ist already started.\n");
        return false;
    }
    else
    {
        if (uwbManager.enableInitiator())
        {
            context.sendResponse("Ringing mode started.\n");
            return true;
        }
        else
        {
            context.sendResponse("Failed to start Ranging mode.\n");
            return false;
        }
    }
}

bool UWBCommand::stopRangeUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    if (configManager.isDeviceTag())
    {
        if (uwbManager.disableInitiator())
        {
            context.sendResponse("Ranging mode stopped.\n");
            return true;
        }
        else
        {
            context.sendResponse("Failed to stop ranging mode.\n");
            return false;
        }
    }
    else
    {
        context.sendResponse("Ranging is already stopped.\n");
        return false;
    }
}
bool UWBCommand::resetUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    uwbManager.resetUWB();
    context.sendResponse("UWB Module restarted.\n");
    return true;
}
bool UWBCommand::printUWBInformationCmd(const std::vector<String> &args, ICommandContext &context)
{
    uwbManager.printRangingInfo();
    return true;
}
bool UWBCommand::configureUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    return false;
}