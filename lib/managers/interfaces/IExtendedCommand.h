#ifndef I_EXTENDED_COMMAND_H
#define I_EXTENDED_COMMAND_H

#include <map>
#include "ICommand.h"

class IExtendedCommand : public ICommand
{
protected:
    using SubCommandHandler = std::function<bool(const std::vector<String> &, ICommandContext &)>;
    std::map<String, SubCommandHandler> subCommands;
    std::map<String, String> subCommandDescriptions;
    std::map<String, std::vector<CommandParameter>> subCommandParameters;

    bool executeSubCommand(const std::vector<String> &args, ICommandContext &context)
    {
        if (args.empty())
        {
            return executeDefaultSubCommand(context);
        }

        String subCommand = args[0];
        std::vector<String> subArgs(args.begin() + 1, args.end());

        auto command = subCommands.find(subCommand);
        if (command == subCommands.end())
        {
            context.sendResponse("Invalid subcommand");
            String response = "Unknown subcommand: " + subCommand + "\nAvailable subcommands:";
            for (const auto &cmd : subCommands)
            {
                response += "\n  " + cmd.first + " - " + getSubCommandDescription(cmd.first);
            }

            context.sendResponse(response.c_str());
            return false;
        }

        if (!validateRequiredParameters(subCommand, subArgs, context))
        {
            return false;
        }

        return command->second(subArgs, context);
    }

    virtual bool executeDefaultSubCommand(ICommandContext &context)
    {
        String response = "Please provide a subcommand\nAvailable subcommands:";
        for (const auto &cmd : subCommands)
        {
            response += "\n  " + cmd.first + " - " + getSubCommandDescription(cmd.first);
        }

        context.sendResponse(response.c_str());
        return true;
    }

    String getNamedParameter(const std::vector<String> &args, const String &paramName, const String &defaultValue = "")
    {
        for (size_t i = 0; i < args.size(); i++)
        {
            if (args[i] == paramName && i + 1 < args.size())
            {
                return args[i + 1];
            }
        }
        return defaultValue;
    }

    bool hasNamedParameter(const std::vector<String> &args, const String &paramName)
    {
        for (auto &arg : args)
        {
            if (arg == paramName)
            {
                return true;
            }
        }
        return false;
    }

    bool validateRequiredParameters(const String &subCommand, const std::vector<String> &args, ICommandContext &context)
    {
        auto paramIterator = subCommandParameters.find(subCommand);
        if (paramIterator == subCommandParameters.end())
        {
            return true;
        }

        String missingParams;
        bool hasMissing = false;

        for (const auto &param : paramIterator->second)
        {
            if (param.required && !hasNamedParameter(args, param.name))
            {
                if (hasMissing)
                    missingParams += ", ";
                missingParams += param.name;
                hasMissing = true;
            }
        }

        if (hasMissing)
        {
            String response = "Missing required parameters: " + missingParams;
            response += "\n\nUsage: " + String(getName()) + " " + subCommand;

            for (const auto &param : paramIterator->second)
            {
                response += "\n  " + param.name + ": " + param.description;
                if (param.required)
                {
                    response += " (required)";
                }
                else if (param.defaultValue.length() > 0)
                {
                    response += " (default: " + param.defaultValue + ")";
                }
            }

            context.sendResponse(response.c_str());
            return false;
        }

        return true;
    }

public:
    bool hasSubCommands() const override { return !subCommands.empty(); }

    std::vector<String> getSubCommands() const override
    {
        std::vector<String> result;
        for (const auto &cmd : subCommands)
        {
            result.push_back(cmd.first);
        }

        return result;
    }

    const char *getSubCommandDescription(const String &subCommand) const override
    {
        auto descriptionIterator = subCommandDescriptions.find(subCommand);
        if (descriptionIterator == subCommandDescriptions.end())
        {
            return "";
        }

        return descriptionIterator->second.c_str();
    }

    std::vector<CommandParameter> getSubCommandParameters(const String &subCommand) const
    {
        auto paramIterator = subCommandParameters.find(subCommand);
        if (paramIterator == subCommandParameters.end())
        {
            return std::vector<CommandParameter>();
        }
        return paramIterator->second;
    }

    bool execute(const std::vector<String> &args, ICommandContext &context) override
    {
        return executeSubCommand(args, context);
    }
};

#endif