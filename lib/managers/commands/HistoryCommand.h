#ifndef HISTORY_COMMAND_H
#define HISTORY_COMMAND_H

#include "interfaces/IExtendedCommand.h"

class CommandManager;

class HistoryCommand : public IExtendedCommand
{
private:
    CommandManager &commandManager;

    bool showCommandHistoryCmd(const std::vector<String> &args, ICommandContext &context);

public:
    HistoryCommand();

    const char *getName() const override
    {
        return "history";
    }

    const char *getDescription() const override
    {
        return "Shows the command history";
    }

    bool execute(const std::vector<String> &args, ICommandContext &context) override;
};

#endif