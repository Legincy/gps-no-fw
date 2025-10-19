#ifndef I_COMMAND_CONTEXT_H
#define I_COMMAND_CONTEXT_H

class ICommandContext
{
public:
    virtual void sendResponse(const char *response) = 0;
    virtual ~ICommandContext() = default;
};

#endif