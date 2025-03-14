#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <map>
#include <memory>
#include "interfaces/ICommand.h"
#include "interfaces/ICommandContext.h"
#include "commands/HelpCommand.h"
#include "commands/PingCommand.h"
#include "commands/WifiCommand.h"
#include "contexts/MQTTCommandContext.h"
#include "managers/MQTTManager.h"
#include "LogManager.h"

class CommandManager
{
private:
    CommandManager()
        : log(LogManager::getInstance()), mqttManager(MQTTManager::getInstance()) {}

    LogManager &log;
    MQTTManager &mqttManager;

    std::map<String, std::shared_ptr<ICommand>> commands;
    std::unique_ptr<MQTTCommandContext> mqttContext;

    void handleMQTTCommand(const char *topic, const uint8_t *payload, unsigned int length);

public:
    CommandManager(const CommandManager &) = delete;
    CommandManager &operator=(const CommandManager &) = delete;

    static CommandManager &getInstance()
    {
        static CommandManager instance;
        return instance;
    }

    bool begin();
    void registerCommand(std::shared_ptr<ICommand> command);
    bool executeCommand(const String &commandStr, ICommandContext &context);
    const std::map<String, std::shared_ptr<ICommand>> &getCommands() const;
    bool hasCommand(const String &name) const;
    std::shared_ptr<ICommand> getCommand(const String &name) const;
};

#endif