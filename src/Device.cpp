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
    updateDistances();
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
void Device::updateDistances()
{
    RuntimeConfig &config = configManager.getRuntimeConfig();
    uint32_t now = millis();

    if (now - lastDistancesUpdate >= config.device.distancesUpdateInterval)
    {
        Cluster cluster = UWBManager::getInstance().getCluster();

        // Für jedes Gerät im Cluster (außer sich selbst) eine Nachricht senden
        for (int i = 0; i < cluster.deviceCount; ++i)
        {
            Node *device = cluster.devices[i];
            // Überspringe das eigene Gerät
            if (strcmp(device->address, config.device.modifiedMac) == 0)
            {
                continue;
            }

            JsonDocument doc;
            if (UWBManager::getInstance().getDistanceJson(doc, device))
            {
                String payload;
                serializeJson(doc, payload);

                char measurementTopic[256];
                snprintf(measurementTopic, sizeof(measurementTopic), "%s/measurements/%s", MQTT_BASE_TOPIC, config.device.modifiedMac);

                // Nachricht mit "isAbsoluteTopic = true" senden
                mqttManager.publish(measurementTopic, payload.c_str(), false, true);
            }
        }
        lastDistancesUpdate = now;
    }
}
void Device::sendDeviceStatus()
{
    JsonDocument doc;

    doc["status"] = "online";
    doc["uptime"] = millis();
    doc["rssi"] = WiFi.RSSI();
    doc["state"] = currentState->getStateIdentifierString();

    JsonObject heap = doc["heap"].to<JsonObject>();
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