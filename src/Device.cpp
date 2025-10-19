#include "Device.h"

bool Device::begin()
{
    log.info("Device", "Initializing Device...");

    if (!configManager.begin())
    {
        log.error("Device", "Failed to initialize ConfigManager");

        return false;
    }
    if (!commandManager.begin())
    {
        log.error("Device", "Failed to initialize CommandManager");
        return false;
    }
    if (!wifiManager.begin() || !wifiManager.connect())
    {
        log.error("Device", "Failed to initialize WifiManager");

        return false;
    }
    log.info("Device", "WiFi initialized successfully");
    if (!mqttManager.begin())
    {
        log.error("Device", "Failed to initialize MQTTManager");

        return false;
    }
    log.info("Device", "Device initialized successfully");
    if (!uwbManager.start())
    {
        log.error("Device", "Failed to initialize UWBManager");

        return false;
    }

    return true;
}
void Device::update()
{
    commandManager.update();
    mqttManager.update();
    uwbManager.update();
}