#include "states/SetupState.h"

void SetupState::enter()
{
    log.debug("SetupState", "Entering SetupState");

    currentPhase = SetupPhase::INIT;
    setupStateTime = millis();
}

void SetupState::update()
{
    switch (currentPhase)
    {
    case SetupPhase::INIT:
        if (!initializeManagers())
        {
            currentPhase = SetupPhase::FAILED;
            return;
        }
        currentPhase = SetupPhase::WIFI_CONNECTING;
        break;
    case SetupPhase::WIFI_CONNECTING:
        handleWifiConnection();
        break;
    case SetupPhase::MQTT_CONNECTING:
        handleMqttConnection();
        // currentPhase = SetupPhase::BLUETOOTH_INIT;
        currentPhase = SetupPhase::COMPLETED;

        break;
    case SetupPhase::BLUETOOTH_INIT:
        if (!bluetoothManager.begin())
        {
            log.error("ActionState", "Failed to initialize BLE");
            return;
        }

        log.info("SetupState", "BLE server started successfully");
        currentPhase = SetupPhase::COMPLETED;
        break;
    case SetupPhase::COMPLETED:
        mqttManager.update();

        if (!commandManager.begin())
        {
            log.error("IdleState", "Failed to initialize CommandManager");
        }
        // TODO: After initialising the command manager, change the output behaviour of the log manager so that non-essential logs are not printed to the serial monitor, but sent to the buffer instead.

        // device->changeState(TestState::getInstance(device));
        // device->changeState(UpdateState::getInstance(device));
        device->changeState(ActionState::getInstance(device));
        break;
    case SetupPhase::FAILED:
        handleSetupFailure();
        break;
    default:
        break;
    }

    /*
    if (millis() - setupStateTime > SETUP_TIMEOUT) {
        log.error("SetupState", "Setup timeout reached");
        currentPhase = SetupPhase::FAILED;
    }
    */
}

void SetupState::exit()
{
    log.debug("SetupState", "Exiting SetupState");
}

bool SetupState::initializeManagers()
{

    if (!wifiManager.begin())
    {
        log.error("SetupState", "Failed to initialize WifiManager");
        return false;
    }

    if (!mqttManager.begin())
    {
        log.error("SetupState", "Failed to initialize MQTTManager");
        return false;
    }
    if (!uwbManager.begin())
    {
        log.error("SetupState", "Failed to initialize UWBManager");
        return false;
    }

    return true;
}

void SetupState::handleWifiConnection()
{
    wifiManager.update();

    switch (wifiManager.getStatus())
    {
    case WiFiStatus::DISCONNECTED:
        wifiManager.connect();
        break;

    case WiFiStatus::CONNECTED:
        log.info("SetupState", "WiFi connected, proceeding to MQTT setup");
        currentPhase = SetupPhase::MQTT_CONNECTING;
        break;

    case WiFiStatus::CONNECTION_FAILED:
    case WiFiStatus::WRONG_PASSWORD:
    case WiFiStatus::NO_SSID_AVAILABLE:
        handleConnectionError("WiFi connection failed", ErrorCode::WIFI_CONNECTION_FAILED);
        break;

    default:
        break;
    }
}

void SetupState::handleMqttConnection()
{
    mqttManager.update();

    if (!mqttManager.isConnected())
    {
        if (!mqttManager.connect())
        {
            RuntimeConfig &config = configManager.getRuntimeConfig();
            mqttConnectionAttempts++;

            if (mqttConnectionAttempts >= config.mqtt.maxConnectionAttempts)
            {
                handleConnectionError("MQTT connection failed", ErrorCode::MQTT_CONNECTION_FAILED);
                return;
            }
            // delay(500);
            return;
        }
    }
    subscribeDefaultTopics();
    currentPhase = SetupPhase::COMPLETED;
}

void SetupState::subscribeDefaultTopics()
{
    RuntimeConfig &config = configManager.getRuntimeConfig();
    // String stationTopic = String(mqttManager.getstationTopic()) + "/#";

    // mqttManager.subscribe(stationTopic.c_str(), [this](const char *topic, const uint8_t *payload, unsigned int length)
    //                       { handleDeviceMessage(topic, payload, length); });

    String topic = String(mqttManager.getStationTopic());
    mqttManager.subscribe(topic.c_str(), [this](const char *topic, const uint8_t *payload, unsigned int length)
                          { handleStationConfig(topic, payload, length); });
}

void SetupState::handleStationConfig(const char *topic, const uint8_t *payload, unsigned int length)
{
    log.info("SetupState", "Received station configuration.");
    char payload_char[length + 1];
    memcpy(payload_char, payload, length);
    payload_char[length] = '\0';

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload_char);
    if (error)
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Failed to parse station config JSON: %s", error.c_str());
        log.error("SetupState", buffer);
        return;
    }

    JsonObject data = doc["data"];
    if (data.isNull())
    {
        log.error("SetupState", "JSON 'data' object not found.");
        return;
    }

    JsonObject config = data["config"];
    JsonObject uwbConfig = config["uwb"];

    if (!uwbConfig.isNull())
    {
        const char *mode = uwbConfig["mode"];
        if (mode)
        {
            UWBManager::getInstance().setDeviceType(mode);
        }
    }

    if (!data["cluster_id"].isNull())
    {
        int clusterId = data["cluster_id"];
        if (ConfigManager::getInstance().setClusterId(clusterId))
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Station belong to cluster with id: %d", clusterId);
            log.info("SetupState", msg);
        }
        else
        {
            log.error("SetupState", "Cannot set clusterId");
        }
    }
    else
    {
        log.info("SetupState", "Device is not assigned to a cluster.");
        uwbManager.changeState(IDLE);
    }
}
// NEU: Funktion zur Verarbeitung der Cluster-Info aus /gpsno/v1/clusters/<ID>
void SetupState::handleClusterConfig(const char *topic, const uint8_t *payload, unsigned int length)
{
    log.info("SetupState", "Received cluster configuration.");
    char payload_char[length + 1];
    memcpy(payload_char, payload, length);
    payload_char[length] = '\0';

    if (uwbManager.updateClusterFromMqtt(payload_char))
    {
        log.info("SetupState", "UWB cluster updated successfully.");
        uwbManager.changeState(CLUSTER_UPDATE);
    }
    else
    {
        log.error("SetupState", "Failed to update UWB cluster.");
    }
}

void SetupState::handleConnectionError(const char *message, ErrorCode errorCode)
{
    log.error("SetupState", message);
    ErrorState::getInstance(device).setError(errorCode, this, message);
    currentPhase = SetupPhase::FAILED;
}

void SetupState::handleSetupFailure()
{
    device->changeState(ErrorState::getInstance(device));
}
