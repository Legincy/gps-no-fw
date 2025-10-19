#include "managers/CommandManager.h"

bool CommandManager::begin()
{
    log.debug("CommandManager", "Initializing command system...");

    // TODO: check for state, cli lock
    registerCommand(std::make_shared<HelpCommand>());
    registerCommand(std::make_shared<PingCommand>());
    registerCommand(std::make_shared<WifiCommand>());
    registerCommand(std::make_shared<BluetoothCommand>());
    registerCommand(std::make_shared<HistoryCommand>());

    showPrompt();

    mqttContext = std::unique_ptr<MQTTCommandContext>(new MQTTCommandContext());

    String mqttCommandTopic = String(mqttManager.getDeviceTopic()) + "/console";
    mqttManager.subscribe(mqttCommandTopic.c_str(), [this](const char *topic, const uint8_t *payload, unsigned int length)
                          { handleMQTTCommand(topic, payload, length); }, false);

    return true;
}

void CommandManager::registerCommand(std::shared_ptr<ICommand> command)
{
    if (!command)
        return;

    String commandName = command->getName();
    commands[commandName] = command;

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Registered command: %s", commandName.c_str());
    log.debug("CommandManager", buffer);
}

void CommandManager::handleMQTTCommand(const char *topic, const uint8_t *payload, unsigned int length)
{
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    String command(message);

    if (command.startsWith("{") && command.endsWith("}"))
    {
        // handleMQTTJsonCommand(command);
    }
    else
    {
        executeCommand(command, *mqttContext);
    }
}

bool CommandManager::executeCommand(const String &commandStr, ICommandContext &context)
{
    std::vector<String> commandArgs;
    String currentArg;
    bool isQuoted = false;

    for (size_t i = 0; i < commandStr.length(); i++)
    {
        char c = commandStr[i];

        if (c == ' ' && !isQuoted)
        {
            if (currentArg.length() > 0)
            {
                commandArgs.push_back(currentArg);
                currentArg = "";
            }
        }
        else if (c == '"')
        {
            if (isQuoted)
            {
                commandArgs.push_back(currentArg);
                currentArg = "";
                isQuoted = false;

                while (i + 1 < commandStr.length() && commandStr[i + 1] == ' ')
                {
                    i++;
                }
            }
            else
            {
                isQuoted = true;
                if (currentArg.length() > 0)
                {
                    commandArgs.push_back(currentArg);
                    currentArg = "";
                }
            }
        }
        else
        {
            currentArg += c;
        }
    }

    if (currentArg.length() > 0)
    {
        commandArgs.push_back(currentArg);
    }

    if (commandArgs.size() == 0)
    {
        log.warning("CommandManager", "No command provided");
        return false;
    }

    String commandName = commandArgs[0];
    commandArgs.erase(commandArgs.begin());

    auto commandIterator = commands.find(commandName);
    if (commandIterator == commands.end())
    {
        context.sendResponse("Unknown command. Type 'help' for available commands.");
        return false;
    }

    return commandIterator->second->execute(commandArgs, context);
}

void CommandManager::showPrompt()
{
    Serial.print("\n\r#> ");
    isPromptDisplayed = true;
}

void CommandManager::update()
{
    if (Serial.available())
    {
        char c = (char)Serial.read();
        processInput(c);
    }
}

void CommandManager::processInput(char c)
{
    if (ignoreNextInputCycle)
    {
        ignoreNextInputCycle = false;
        return;
    }

    if (c == '\n' || c == '\r')
    {
        handleEnter();
    }
    else if (c == 127 || c == 8)
    {
        handleBackspace();
    }
    else
    {
        inputBuffer += c;
        Serial.print(c);
    }
}

void CommandManager::handleEnter()
{
    ignoreNextInputCycle = true;

    if (inputBuffer.length() > 0)
    {
        Serial.println();
        addCommandToHistory(inputBuffer.c_str());
        executeCommand(inputBuffer, context);
        inputBuffer = "";
    }

    showPrompt();
}

void CommandManager::handleBackspace()
{
    if (inputBuffer.length() > 0)
    {
        inputBuffer.remove(inputBuffer.length() - 1);
        Serial.print("\b \b");
    }
}

void CommandManager::addCommandToHistory(const char *command)
{
    char buffer[COMMAND_LINE_LENGTH];

    if (strlen(command) >= COMMAND_LINE_LENGTH)
    {
        strncpy(buffer, command, COMMAND_LINE_LENGTH - 4);
        buffer[COMMAND_LINE_LENGTH - 4] = '.';
        buffer[COMMAND_LINE_LENGTH - 3] = '.';
        buffer[COMMAND_LINE_LENGTH - 2] = '.';
        buffer[COMMAND_LINE_LENGTH - 1] = '\0';
    }
    else
    {
        strcpy(buffer, command);
    }

    if (commandHistoryIndex >= COMMAND_HISTORY_SIZE)
    {
        for (size_t i = 0; i < COMMAND_HISTORY_SIZE - 1; i++)
        {
            strcpy(commandHistory[i], commandHistory[i + 1]);
        }

        strcpy(commandHistory[COMMAND_HISTORY_SIZE - 1], buffer);
    }
    else
    {
        strcpy(commandHistory[commandHistoryIndex], buffer);
    }

    commandHistoryIndex++;
}

void CommandManager::showCommandHistory()
{
    for (size_t i = 0; i < COMMAND_HISTORY_SIZE; i++)
    {
        Serial.println(commandHistory[i]);
    }
}

void CommandManager::clearCommandHistory()
{

    commandHistoryIndex = 0;
    memset(commandHistory, 0, sizeof(commandHistory));
}

const std::map<String, std::shared_ptr<ICommand>> &CommandManager::getCommands() const
{
    return commands;
}

bool CommandManager::hasCommand(const String &name) const
{
    return commands.find(name) != commands.end();
}

std::shared_ptr<ICommand> CommandManager::getCommand(const String &name) const
{
    auto commandIterator = commands.find(name);
    if (commandIterator == commands.end())
    {
        return nullptr;
    }

    return commandIterator->second;
}