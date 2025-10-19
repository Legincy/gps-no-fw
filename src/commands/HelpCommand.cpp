#include "commands/HelpCommand.h"

bool HelpCommand::execute(const std::vector<String> &args, ICommandContext &context)
{
    CommandManager &commandManager = CommandManager::getInstance();

    if (!args.empty())
    {
        String commandName = args[0];
        if (commandManager.hasCommand(commandName))
        {
            auto cmd = commandManager.getCommand(commandName);
            String response = "Help for command '" + commandName + "':\n";
            response += "  Description: " + String(cmd->getDescription()) + "\n";

            if (cmd->hasSubCommands())
            {
                response += "\nSubcommands:\n";
                std::vector<String> subCommands = cmd->getSubCommands();

                size_t maxSubCmdLength = 0;
                for (const auto &subCmd : subCommands)
                {
                    if (subCmd.length() > maxSubCmdLength)
                    {
                        maxSubCmdLength = subCmd.length();
                    }
                }
                maxSubCmdLength += 4;

                for (const auto &subCmd : subCommands)
                {
                    String paddedSubCmd = subCmd;
                    while (paddedSubCmd.length() < maxSubCmdLength)
                    {
                        paddedSubCmd += " ";
                    }

                    response += "  " + paddedSubCmd + "- " + String(cmd->getSubCommandDescription(subCmd)) + "\n";

                    std::vector<CommandParameter> params = cmd->getSubCommandParameters(subCmd);
                    if (!params.empty())
                    {
                        response += "    Parameters:\n";

                        size_t maxParamLength = 0;
                        for (const auto &param : params)
                        {
                            if (param.name.length() > maxParamLength)
                            {
                                maxParamLength = param.name.length();
                            }
                        }
                        maxParamLength += 4;

                        for (const auto &param : params)
                        {
                            String paddedParam = param.name;
                            while (paddedParam.length() < maxParamLength)
                            {
                                paddedParam += " ";
                            }

                            response += "      " + paddedParam + "- " + param.description;
                            if (param.required)
                            {
                                response += " (required)";
                            }
                            else if (param.defaultValue.length() > 0)
                            {
                                response += " (default: " + param.defaultValue + ")";
                            }
                            response += "\n";
                        }
                    }
                }
            }

            context.sendResponse(response.c_str());
            return true;
        }
        else
        {
            String response = "Unknown command '" + commandName + "'. Type 'help' for a list of commands.";
            context.sendResponse(response.c_str());
            return false;
        }
    }

    String response = "Available commands:\n";
    const auto &commands = commandManager.getCommands();

    size_t maxLength = 0;
    for (const auto &cmd : commands)
    {
        if (cmd.first.length() > maxLength)
        {
            maxLength = cmd.first.length();
        }
    }

    maxLength += 4;

    for (const auto &cmd : commands)
    {
        String paddedCommand = cmd.first;

        while (paddedCommand.length() < maxLength)
        {
            paddedCommand += " ";
        }

        response += "  " + paddedCommand + "- " + cmd.second->getDescription() + "\n";
    }

    response += "\nType 'help <command>' for detailed information about a specific command.";

    context.sendResponse(response.c_str());
    return true;
}