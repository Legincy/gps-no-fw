#include "commands/WifiCommand.h"

bool WifiCommand::startAccessPointCmd(const std::vector<String>& args, ICommandContext& context) {
    if (args.size() < 2) {
        context.sendResponse("Please provide an SSID and password\n");
        return false;
    }

    String ssid = args[1];
    String password = args.size() > 2 ? args[2] : "";

    if (!wifiManager.ftmAP(ssid.c_str(), password.c_str())) {
        context.sendResponse("Failed to start access point\n");
        return false;
    }

    context.sendResponse("Access point started\n");

    return true;
}

bool WifiCommand::scanNetworksCmd(const std::vector<String>& args, ICommandContext& context) {
    bool ftm = false;
    if (args.size() > 1) {
        ftm = args[1] == "true";
    }

    if (!wifiManager.scan(ftm)) {
        context.sendResponse("Failed to scan networks\n");
        return false;
    }
    return true;
}

bool WifiCommand::initiateFTMCmd(const std::vector<String>& args, ICommandContext& context) {
    
    byte mac[6];
    int channel;

    if (args.size() < 3) {
        context.sendResponse("Please provide a MAC address and channel\n");
        return false;
    }

    sscanf(args[3].c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &mac[0], &mac[1], &mac[2], 
           &mac[3], &mac[4], &mac[5]);
    
    channel = args[1].toInt();

    Serial.printf("Initiating FTM for channel %d and MAC %02x:%02x:%02x:%02x:%02x:%02x with frame count %d and burst period %d ms\n",
                  channel, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], configManager.getRuntimeConfig().wifi.ftmFrameCount, configManager.getRuntimeConfig().wifi.ftmBurstPeriod * 100);

    WifiManager::getInstance().initiateFtm(channel, mac);

    Serial.printf("FTM Status: %s, Distance: %.2f\n", WifiManager::getInstance().getFtmStatusString(), (float)WifiManager::getInstance().getFtmDistance() / 100.0);

    return true;
}