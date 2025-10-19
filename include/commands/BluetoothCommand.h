#ifndef BLUETOOTH_COMMAND_H
#define BLUETOOTH_COMMAND_H

#include "interfaces/IExtendedCommand.h"
#include "managers/BluetoothManager.h"
#include "managers/ConfigManager.h"

class BluetoothCommand : public IExtendedCommand
{
private:
    BluetoothManager &bluetoothManager;
    ConfigManager &configManager;
    LogManager &log;

    bool startBluetoothServerCmd(const std::vector<String> &args, ICommandContext &context);

public:
    BluetoothCommand()
        : bluetoothManager(BluetoothManager::getInstance()), log(LogManager::getInstance()), configManager(ConfigManager::getInstance())
    {
        subCommands["start"] = [this](const std::vector<String> &args, ICommandContext &context)
        {
            startBluetoothServerCmd(args, context);
            return true;
        };
        subCommandDescriptions["start"] = "Starts the Bluetooth server";
        subCommandParameters["start"] = {
            {"--name", "Name of the device", false, ""},
            {"--service", "Service UUID", false, ""},
            {"--characteristic", "Characteristic UUID", false, ""}};
    }

    const char *getName() const override
    {
        return "bluetooth";
    }

    const char *getDescription() const override
    {
        return "Configure and manage Bluetooth";
    }
};

#endif