#include "Device.h"

bool Device::begin()
{
    log.info("Device", "Initializing Device...");

    if (!configManager.begin())
    {
        log.error("Device", "Failed to initialize ConfigManager");

        return false;
    }

    // TODO: Redo the integry check of stored config vs config defines
    if (configManager.hasConfigDefinesChanged())
    {
        log.info("Device", "ConfigDefines has changed, updating device config");
        configManager.updateDeviceConfig();
    }

    return true;
}

void Device::changeState(IDeviceState &newState)
{
    if (currentState)
    {
        currentState->exit();
    }

    currentState = &newState;

    if (currentState)
    {
        currentState->enter();
    }
}

void Device::update()
{
    commandManager.update();

    if (!currentState)
    {
        log.error("Device", "No current state set");
        return;
    }

    updateDeviceStatus();
    currentState->update();
}

void Device::updateDeviceStatus()
{
    RuntimeConfig &config = configManager.getRuntimeConfig();
    uint32_t now = millis();

    if (now - lastStatusUpdate >= config.device.statusUpdateInterval)
    {
        sendDeviceStatus();
        lastStatusUpdate = now;
    }
}

void Device::sendDeviceStatus()
{
    DynamicJsonDocument doc(2048);

    doc["status"] = "online";
    doc["uptime"] = millis();
    doc["rssi"] = WiFi.RSSI();
    doc["state"] = currentState->getStateIdentifierString();

    JsonObject heap = doc.createNestedObject("heap");
    heap["free"] = ESP.getFreeHeap();
    heap["min_free"] = ESP.getMinFreeHeap();
    heap["max_alloc"] = ESP.getMaxAllocHeap();

    String payload;
    serializeJson(doc, payload);

    mqttManager.publish("status", payload.c_str(), false);
}

const char *Device::getDeviceStatusString(DeviceStatus status)
{
    switch (status)
    {
    case DeviceStatus::BOOTING:
        return "BOOTING";
    case DeviceStatus::IDLE:
        return "IDLE";
    case DeviceStatus::SETUP:
        return "SETUP";
    case DeviceStatus::TRANSITIONING:
        return "TRANSITIONING";
    case DeviceStatus::ACTION:
        return "ACTION";
    case DeviceStatus::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}