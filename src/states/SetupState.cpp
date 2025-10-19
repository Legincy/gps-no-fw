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

        device->changeState(TestState::getInstance(device));
        // device->changeState(UpdateState::getInstance(device));
        // evice->changeState(ActionState::getInstance(device));
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
            delay(500);
            return;
        }
    }

    // subscribeDefaultTopics();
}

void SetupState::subscribeDefaultTopics()
{
    RuntimeConfig &config = configManager.getRuntimeConfig();
    String deviceTopic = String(mqttManager.getDeviceTopic()) + "/#";

    mqttManager.subscribe(deviceTopic.c_str(), [this](const char *topic, const uint8_t *payload, unsigned int length)
                          { handleDeviceMessage(topic, payload, length); });

    String configTopic = String(mqttManager.getDeviceTopic()) + "/config";
    mqttManager.subscribe(configTopic.c_str(), [this](const char *topic, const uint8_t *payload, unsigned int length)
                          { handleConfigMessage(topic, payload, length); });
}

void SetupState::handleDeviceMessage(const char *topic, const uint8_t *payload, unsigned int length)
{
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    char logMessage[1024];
    snprintf(logMessage, sizeof(logMessage), "Received message on topic %s: %s", topic, message);
    log.debug("SetupState", logMessage);
}

void SetupState::handleConfigMessage(const char *topic, const uint8_t *payload, unsigned int length)
{
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    log.info("SetupState", "Received config update");
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
