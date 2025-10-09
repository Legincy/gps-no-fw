#include "commands/UWBCommand.h"

bool UWBCommand::startUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    context.sendResponse("Starting Ultra Wideband module...\n");
    // if (uwbManager.startUWB())
    // {

    // }
    // else
    // {
    //     context.sendResponse("Failed to start UWB Manager.\n");
    // }
    return true;
}

bool UWBCommand::stopUWBCmd(const std::vector<String> &args, ICommandContext &context)
{
    if (uwbTaskHandle != NULL)
    {
        vTaskDelete(uwbTaskHandle);
        uwbTaskHandle = NULL;
        context.sendResponse("UWB loop task stopped.\n");
    }
    context.sendResponse("Ultra Wideband module stopped.\n");
    return true;
}
