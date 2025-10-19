#include "commands/BluetoothCommand.h"

bool BluetoothCommand::startBluetoothServerCmd(const std::vector<String> &args, ICommandContext &context)
{
    String name = getNamedParameter(args, "--name");
    String serviceUUID = getNamedParameter(args, "--service");
    String characteristicUUID = getNamedParameter(args, "--characteristic");

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Starting Bluetooth server with name: %s, service UUID: %s, characteristic UUID: %s", name.c_str(), serviceUUID.c_str(), characteristicUUID.c_str());
    log.info("BluetoothCommand", buffer);

    return true;
}