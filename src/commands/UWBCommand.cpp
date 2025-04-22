#include "commands/UWBCommand.h"

bool UWBCommand::startUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    context.sendResponse("Ultra Wideband module started\n");
    uwbManager.changeState(CLUSTER_UPDATE);
    return true;
}

bool UWBCommand::stopUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    uwbManager.changeState(IDLE);
    context.sendResponse("Ultra Wideband module stopped\n");
    return true;
}
