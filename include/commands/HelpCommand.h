#ifndef HELP_COMMAND_H
#define HELP_COMMAND_H

#include <map>
#include "interfaces/ICommand.h"
#include "interfaces/IExtendedCommand.h"
#include "managers/CommandManager.h"

class HelpCommand : public ICommand
{
public:
    HelpCommand() {}

    const char *getName() const override
    {
        return "help";
    }

    const char *getDescription() const override
    {
        return "Lists all available commands";
    }

    bool execute(const std::vector<String> &args, ICommandContext &context) override;

private:
};

#endif