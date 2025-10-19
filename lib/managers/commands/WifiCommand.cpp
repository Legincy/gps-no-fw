#include "commands/WifiCommand.h"

bool WifiCommand::startAccessPointCmd(const std::vector<String> &args, ICommandContext &context)
{
    if (args.size() < 2)
    {
        context.sendResponse("Please provide an SSID and password\n");
        return false;
    }

    String ssid = args[1];
    String password = args.size() > 2 ? args[2] : "";
    context.sendResponse("Access point started\n");

    return true;
}

bool WifiCommand::scanNetworksCmd(const std::vector<String> &args, ICommandContext &context)
{

    return true;
}

bool WifiCommand::initiateFTMCmd(const std::vector<String> &args, ICommandContext &context)
{
    return true;
}