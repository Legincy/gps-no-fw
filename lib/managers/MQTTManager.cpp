#include "MQTTManager.h"
#include <cstring>

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
    if (_mqttServerIp == nullptr || strlen(_mqttServerIp) == 0)
    {
        log.error("MQTTManager", "MQTT broker address is not defined");
        return false;
    }

    _mqttClient.setServer(_mqttServerIp, _mqttServerPort);
    _mqttClient.setBufferSize(2048);
    _mqttClient.setCallback(handleCallback);
    // _mqttClient.qoS
    _mqttClient.setKeepAlive(_keepAlive);
    log.info("MQTTManager", "MQTT client initialized successfully");

    return true;
}

void MQTTManager::connect()
{
    while (!_mqttClient.connected())
    {
        log.info("MQTTManager", "Attempting MQTT connection....");
        /// Connect to MQTT Broker
        bool connected = false;

        // Wenn Username und Passwort leer sind, ohne Authentifizierung verbinden
        if (strlen(_mqttUsername) == 0 && strlen(_mqttPassword) == 0)
        {
            connected = _mqttClient.connect(_clientId);
        }
        else
        {
            connected = _mqttClient.connect(_clientId, _mqttUsername, _mqttPassword);
        }

        if (connected)
        {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Connected to MQTT Broker %s, state: %d", MQTT_BROKER, _mqttClient.state());
            log.info("MQTTManager", buffer);
            // _mqttClient.subscribe("/test", _qos);
        }
        else
        {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Connection to Broker %s failed, state: %d", MQTT_BROKER, _mqttClient.state());
            log.error("MQTTManager", buffer);
            log.error("MQTTManager", "Trying again in 5 secounds");
            log.delay(5000);
        }
    }
}

void MQTTManager::disconnect()
{
    if (_mqttClient.connected())
    {
        _mqttClient.disconnect();
        log.info("MQTTManager", "Disconnected from MQTT broker");
    }
}

bool MQTTManager::publish(const char *topic, const char *payload, bool retain, bool isAbsoluteTopic)
{
    if (!_mqttClient.connected())
    {
        log.error("MQTTManager", "MQTT client not connected");
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
        log.info("MQTTManager", logMsg);
    }
    else
    {
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "Failed to publish message to topic: %s", fullTopic);
        log.error("MQTTManager", logMsg);
    }
    return success;
}

bool MQTTManager::subscribe(const char *topic, MQTTCallback callback)
{
    if (!_mqttClient.connected())
    {
        log.error("MQTTManager", "MQTT client not connected");
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
        log.info("MQTTManager", logMsg);
    }
    else
    {
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "Failed to subscribe to topic: %s", topic);
        log.error("MQTTManager", logMsg);
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
