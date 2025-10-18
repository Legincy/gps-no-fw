#include "MQTTManager.h"
#include <cstring>

MQTTManager::MQTTManager()
    : logManager(LogManager::getInstance()),
      configManager(ConfigManager::getInstance()),
      runtimeConfig(configManager.getRuntimeConfig()),
      _wifiClient(),
      _mqttClient(_wifiClient)
{
    // Die Initialisierung der Member erfolgt in der Initialisierungsliste.
    // Der Konstruktor-Body ist leer. Die eigentliche Logik kommt in begin().
}

void MQTTManager::handleCallback(char *topic, byte *payload, unsigned int length)
{
    // Convert payload into a null-terminated string
    char *message = new char[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    // Log the payload using LogManager
    LogManager::getInstance().info("MQTTManager", message);

    delete[] message;
}

bool MQTTManager::begin()
{
    if (runtimeConfig.mqtt.broker == 0)
    {
        logManager.error("MQTTManager", "MQTT broker address is not defined");
        return false;
    }

    _mqttClient.setServer(runtimeConfig.mqtt.broker, runtimeConfig.mqtt.port);
    _mqttClient.setBufferSize(2048);
    _mqttClient.setCallback(handleCallback);
    // _mqttClient.qoS
    _mqttClient.setKeepAlive(MQTT_KEEP_ALIVE);
    logManager.info("MQTTManager", "MQTT client initialized successfully");

    return true;
}

void MQTTManager::connect()
{
    while (!_mqttClient.connected())
    {
        logManager.info("MQTTManager", "Attempting MQTT connection....");
        /// Connect to MQTT Broker
        bool connected = false;

        // Wenn Username und Passwort leer sind, ohne Authentifizierung verbinden
        if (strlen(runtimeConfig.mqtt.user) == 0 && strlen(runtimeConfig.mqtt.password) == 0)
        {
            connected = _mqttClient.connect(_clientId);
        }
        else
        {
            connected = _mqttClient.connect(_clientId, runtimeConfig.mqtt.user, runtimeConfig.mqtt.password);
        }

        if (connected)
        {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Connected to MQTT Broker %s, state: %d", runtimeConfig.mqtt.broker, _mqttClient.state());
            logManager.info("MQTTManager", buffer);
            // _mqttClient.subscribe("/test", _qos);
        }
        else
        {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Connection to Broker %s failed, state: %d", runtimeConfig.mqtt.broker, _mqttClient.state());
            logManager.error("MQTTManager", buffer);
            logManager.error("MQTTManager", "Trying again in 5 secounds");
            logManager.delay(5000);
        }
    }
}

void MQTTManager::disconnect()
{
    if (_mqttClient.connected())
    {
        _mqttClient.disconnect();
        logManager.info("MQTTManager", "Disconnected from MQTT broker");
    }
}

bool MQTTManager::publish(const char *topic, const char *payload, bool retain, bool isAbsoluteTopic)
{
    if (!_mqttClient.connected())
    {
        logManager.error("MQTTManager", "MQTT client not connected");
        return false;
    }

    char fullTopic[512];
    if (isAbsoluteTopic)
    {
        snprintf(fullTopic, sizeof(fullTopic), "/%s", topic);
    }
    else
    {
        snprintf(fullTopic, sizeof(fullTopic), "%s/%s", _pubTopic, topic);
    }

    bool success = _mqttClient.publish(fullTopic, payload, retain);
    if (success)
    {
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "Published message to topic: %s", fullTopic);
        logManager.info("MQTTManager", logMsg);
    }
    else
    {
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "Failed to publish message to topic: %s", fullTopic);
        logManager.error("MQTTManager", logMsg);
    }
    return success;
}

bool MQTTManager::subscribe(const char *topic, MQTTCallback callback)
{
    if (!_mqttClient.connected())
    {
        logManager.error("MQTTManager", "MQTT client not connected");
        return false;
    }

    bool success = _mqttClient.subscribe(topic);
    if (success)
    {
        Subscription sub;
        sub.topic = topic;
        sub.callback = callback;
        _subscriptions.push_back(sub);
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "Subscribed to topic: %s", topic);
        logManager.info("MQTTManager", logMsg);
    }
    else
    {
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "Failed to subscribe to topic: %s", topic);
        logManager.error("MQTTManager", logMsg);
    }
    return success;
}

void MQTTManager::update()
{
    if (!_mqttClient.connected())
    {
        connect();
    }
    _mqttClient.loop();
}

bool MQTTManager::publishMeasurement(const char *payload)
{
    const char *macAddress = runtimeConfig.device.modifiedMac;
    char fullTopic[128];
    snprintf(fullTopic, sizeof(fullTopic), "%s/measurements/%s", MQTT_BASE_TOPIC, macAddress);

    return publish(fullTopic, payload, false, true);
}

bool MQTTManager::publishConfig(const char *payload)
{
    const char *macAddress = runtimeConfig.device.modifiedMac;
    char fullTopic[128];
    snprintf(fullTopic, sizeof(fullTopic), "%s/stations/%s", MQTT_BASE_TOPIC, macAddress);

    return publish(fullTopic, payload, true, true);
}
