#include "commands/HistoryCommand.h"
#include "managers/CommandManager.h"

HistoryCommand::HistoryCommand() : commandManager(CommandManager::getInstance())
{
    subCommands["show"] = [this](const std::vector<String> &args, ICommandContext &context)
    {
        return showCommandHistoryCmd(args, context);
    };
    subCommandDescriptions["show"] = "Shows the command history";

    subCommands["clear"] = [this](const std::vector<String> &args, ICommandContext &context)
    {
        commandManager.clearCommandHistory();

        context.sendResponse("Command history cleared");
        return true;
    };
    subCommandDescriptions["clear"] = "Clears the command history";
}

bool HistoryCommand::showCommandHistoryCmd(const std::vector<String> &args, ICommandContext &context)
{
    auto commandHistory = commandManager.getCommandHistory();
    size_t commandHistoryIndex = commandManager.getCommandHistoryIndex();

    char buffer[COMMAND_HISTORY_SIZE * (COMMAND_LINE_LENGTH + 64)];
    buffer[0] = '\0';

    int validEntries = 0;

    for (int i = 0; i < COMMAND_HISTORY_SIZE; i++)
    {
        if (commandHistory[i][0] != '\0')
        {
            validEntries++;
        }
    }

    if (validEntries == 0)
    {
        strcat(buffer, "No commands in history.\n");
    }
    else
    {
        for (int i = 0; i < COMMAND_HISTORY_SIZE; i++)
        {
            if (commandHistory[i][0] != '\0')
            {
                char entryBuffer[COMMAND_LINE_LENGTH + 16];
                size_t commandIndex = i;

                if (commandHistoryIndex >= COMMAND_HISTORY_SIZE)
                {
                    commandIndex = commandHistoryIndex - COMMAND_HISTORY_SIZE + i;
                }

                snprintf(entryBuffer, sizeof(entryBuffer), "%2d: %s\n", commandIndex, commandHistory[i]);
                strcat(buffer, entryBuffer);
            }
        }
    }

    context.sendResponse(buffer);
    return true;
}

bool HistoryCommand::execute(const std::vector<String> &args, ICommandContext &context)
{
    if (args.empty())
    {
        return showCommandHistoryCmd(args, context);
    }

    return executeSubCommand(args, context);
}