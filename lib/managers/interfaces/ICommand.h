#ifndef I_COMMAND_H
#define I_COMMAND_H

#include <ArduinoJson.h>
#include <vector>
#include "ICommandContext.h"

struct CommandParameter
{
    String name;
    String description;
    bool required;
    String defaultValue;
};

class ICommand
{
public:
    virtual const char *getName() const = 0;
    virtual const char *getDescription() const = 0;

    virtual bool execute(const std::vector<String> &args, ICommandContext &context) = 0;
    virtual bool hasSubCommands() const { return false; }
    virtual std::vector<String> getSubCommands() const { return {}; }
    virtual const char *getSubCommandDescription(const String &subCommand) const { return ""; }

    virtual std::vector<CommandParameter> getSubCommandParameters(const String &subCommand) const { return {}; }

    virtual ~ICommand() = default;
};

#endif