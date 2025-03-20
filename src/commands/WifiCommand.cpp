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
    int frameCount = configManager.getRuntimeConfig().wifi.ftmFrameCount;
    int burstPeriod = configManager.getRuntimeConfig().wifi.ftmBurstPeriod;

    for (int i = 0; i < args.size(); i++) {
        if (args[i] == "--channel") {
            channel = args[i + 1].toInt();
        } else if (args[i] == "--mac") {
            sscanf(args[i + 1].c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
                   &mac[0], &mac[1], &mac[2], 
                   &mac[3], &mac[4], &mac[5]);
        } else if (args[i] == "--framecount") {
            frameCount = args[i + 1].toInt();
        } else if (args[i] == "--burstperiod") {
            burstPeriod = args[i + 1].toInt();
        }
    }

    if (mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0) {
        context.sendResponse("Please provide a MAC address\n");
        return false;
    }

    if (channel == 0) {
        context.sendResponse("Please provide a channel\n");
        return false;
    }

    Serial.printf("Initiating FTM for channel %d and MAC %02x:%02x:%02x:%02x:%02x:%02x with frame count %d and burst period %d ms\n",
                  channel, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], frameCount, burstPeriod * 100);

    WifiManager::getInstance().initiateFtm(channel, mac);

    Serial.printf("FTM Status: %s, Distance: %.2f\n", WifiManager::getInstance().getFtmStatusString(), (float)WifiManager::getInstance().getFtmDistance() / 100.0);

    return true;
}