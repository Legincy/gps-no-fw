#ifndef PING_COMMAND_H
#define PING_COMMAND_H

#include "interfaces/ICommand.h"

class PingCommand : public ICommand
{
public:
    PingCommand() {}

    const char *getName() const override
    {
        return "ping";
    }

    const char *getDescription() const override
    {
        return "Ping the server.";
    }

    bool execute(const std::vector<String> &args, ICommandContext &context) override
    {
        context.sendResponse("Pong!");
        return true;
    }
};

#endif