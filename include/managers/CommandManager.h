#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#define COMMAND_HISTORY_SIZE 20
#define COMMAND_LINE_LENGTH 128

#include <map>
#include <memory>
#include "interfaces/ICommand.h"
#include "interfaces/ICommandContext.h"
#include "commands/HelpCommand.h"
#include "commands/PingCommand.h"
#include "commands/WifiCommand.h"
#include "commands/BluetoothCommand.h"
#include "commands/HistoryCommand.h"
#include "contexts/MQTTCommandContext.h"
#include "contexts/SerialCommandContext.h"
#include "managers/MQTTManager.h"
#include "LogManager.h"

struct SerialCommand
{
    char command[128];
};

class CommandManager
{
private:
    CommandManager()
        : log(LogManager::getInstance()), mqttManager(MQTTManager::getInstance()), isLocked(true), commandHistoryIndex(0)
    {
        for (int i = 0; i < COMMAND_HISTORY_SIZE; i++)
        {
            commandHistory[i][0] = '\0';
        }
    }

    LogManager &log;
    MQTTManager &mqttManager;

    bool isLocked;
    char commandHistory[COMMAND_HISTORY_SIZE][COMMAND_LINE_LENGTH];
    size_t commandHistoryIndex;

    std::map<String, std::shared_ptr<ICommand>> commands;
    std::unique_ptr<MQTTCommandContext> mqttContext;

    SerialCommandContext context;
    String inputBuffer;
    bool isPromptDisplayed;
    const char *prompt;
    bool ignoreNextInputCycle;

    void handleMQTTCommand(const char *topic, const uint8_t *payload, unsigned int length);
    void processInput(char c);
    void showPrompt();
    void handleEnter();
    void handleBackspace();
    void addCommandToHistory(const char *command);
    void showCommandHistory();

public:
    CommandManager(const CommandManager &) = delete;
    CommandManager &operator=(const CommandManager &) = delete;

    static CommandManager &getInstance()
    {
        static CommandManager instance;
        return instance;
    }

    bool begin();
    void update();
    void registerCommand(std::shared_ptr<ICommand> command);
    bool executeCommand(const String &commandStr, ICommandContext &context);

    char (*getCommandHistory())[COMMAND_LINE_LENGTH] { return commandHistory; };
    void clearCommandHistory();
    size_t getCommandHistoryIndex() { return commandHistoryIndex; };
    const std::map<String, std::shared_ptr<ICommand>> &getCommands() const;
    bool hasCommand(const String &name) const;
    std::shared_ptr<ICommand> getCommand(const String &name) const;
};

#endif