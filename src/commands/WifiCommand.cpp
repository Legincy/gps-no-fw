#include "commands/WifiCommand.h"

bool WifiCommand::startAccessPointCmd(const std::vector<String> &args, ICommandContext &context)
{
    if (args.size() < 2)
    {
        context.sendResponse("Please provide an SSID and password\n");
        return false;
    }

    String ssid = args[1];
    String password = args.size() > 2 ? args[2] : "";

    if (!wifiManager.ftmAP(ssid.c_str(), password.c_str()))
    {
        context.sendResponse("Failed to start access point\n");
        return false;
    }

    context.sendResponse("Access point started\n");

    return true;
}

bool WifiCommand::scanNetworksCmd(const std::vector<String> &args, ICommandContext &context)
{
    bool ftm = false;
    if (args.size() > 1)
    {
        ftm = args[1] == "true";
    }

    if (!wifiManager.scan(ftm))
    {
        context.sendResponse("Failed to scan networks\n");
        return false;
    }
    return true;
}

bool WifiCommand::initiateFTMCmd(const std::vector<String> &args, ICommandContext &context)
{

    byte mac[6];
    int channel;

    if (args.size() < 3)
    {
        context.sendResponse("Please provide a MAC address and channel\n");
        return false;
    }

    sscanf(args[1].c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac[0], &mac[1], &mac[2],
           &mac[3], &mac[4], &mac[5]);

    channel = args[3].toInt();

    Serial.printf("Channel: %d\n", channel);
    Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2],
                  mac[3], mac[4], mac[5]);

    WifiManager::getInstance().initiateFtm(channel, mac);

    return true;
}