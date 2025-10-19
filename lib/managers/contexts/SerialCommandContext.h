#ifndef SERIAL_COMMAND_CONTEXT_H
#define SERIAL_COMMAND_CONTEXT_H

#include <Arduino.h>
#include "interfaces/ICommandContext.h"

class SerialCommandContext : public ICommandContext
{
public:
    void sendResponse(const char *response) override
    {
        Serial.print(response);
    }
};

#endif