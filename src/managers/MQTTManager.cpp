#include "managers/MQTTManager.h"

void MQTTManager::handleCallback(char *topic, uint8_t *payload, uint32_t length)
{
    char *message = new char[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer), "Received message on topic '%s': '%s'", topic, message);
    log.debug("MQTTManager", msgBuffer);

    // log.debug("MQTTManager", "Checking subscriptions:");
    for (const auto &subscription : subscriptions)
    {
        char subBuffer[128];
        snprintf(subBuffer, sizeof(subBuffer), "Checking against subscription: '%s'", subscription.topic.c_str());
        // log.debug("MQTTManager", subBuffer);

        if (matchTopic(subscription.topic.c_str(), topic))
        {
            subscription.callback(topic, payload, length);
        }
    }

    delete[] message;
}

bool MQTTManager::isSubscribed(const char *topic)
{
    for (const Subscription &subscription : subscriptions)
    {
        if (subscription.topic == topic)
        {
            return true;
        }
    }

    return false;
}

bool MQTTManager::matchTopic(const char *pattern, const char *topic)
{
    while (*pattern && *topic)
    {
        if (*pattern == '+')
        {
            while (*topic && *topic != '/')
                topic++;
            pattern++;
            if (*topic)
                topic++;
            if (*pattern)
                pattern++;
            continue;
        }
        if (*pattern == '#')
        {
            return true;
        }
        if (*pattern != *topic)
            return false;
        pattern++;
        topic++;
    }
    return *pattern == *topic;
}

bool MQTTManager::begin()
{
    if (initialized)
    {
        return true;
    }

    RuntimeConfig &config = configManager.getRuntimeConfig();

    if (strlen(config.mqtt.broker) == 0)
    {
        log.warning("MQTTManager", "No MQTT broker available, skipping MQTTManager initialization");
        return false;
    }
    else if (config.mqtt.port == 0)
    {
        log.warning("MQTTManager", "No MQTT port available, skipping MQTTManager initialization");
        return false;
    }

    client.setServer(config.mqtt.broker, config.mqtt.port);
    client.setBufferSize(2048);
    client.setCallback([this](char *topic, byte *payload, unsigned int length)
                       { handleCallback(topic, payload, length); });

    initialized = true;

    return true;
}

bool MQTTManager::connect()
{
    if (!initialized)
    {
        log.error("MQTTManager", "MQTTManager not initialized");
        return false;
    }

    if (client.connected())
        return true;

    RuntimeConfig &config = configManager.getRuntimeConfig();
    bool connectionResult;

    char msgBuffer[256];
    snprintf(msgBuffer, sizeof(msgBuffer), "Attempting to connect to MQTT-Broker '%s' (['%s', %d], ['%s', %d])", config.mqtt.broker, config.mqtt.user, strlen(config.mqtt.user), config.mqtt.password, strlen(config.mqtt.password));
    log.debug("MQTTManager", msgBuffer);

    if (strlen(config.mqtt.user) > 0)
    {
        connectionResult = client.connect(clientId, config.mqtt.user, config.mqtt.password);
    }
    else
    {
        connectionResult = client.connect(clientId);
    }

    if (connectionResult)
    {
        log.info("MQTTManager", "Connected to MQTT broker");
        handleSubscriptions();

        return true;
    }
    else
    {
        char msgBuffer[64];
        snprintf(msgBuffer, sizeof(msgBuffer), "Connection failed, rc=%d", client.state());
        log.error("MQTTManager", msgBuffer);
        return false;
    }
}

void MQTTManager::disconnect()
{
    if (client.connected())
    {
        client.disconnect();
        log.info("MQTTManager", "Disconnected from MQTT broker");
    }
}

bool MQTTManager::subscribe(const char *topic, MQTTCallback callback, bool isPersistent)
{
    for (const auto &subscription : subscriptions)
    {
        if (subscription.topic == topic)
        {
            log.warning("MQTTManager", "Already subscribed to topic");
            return false;
        }
    }

    subscriptions.push_back({String(topic), callback, isPersistent, false});

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer), "Subscribed to topic: %s", topic);
    log.info("MQTTManager", msgBuffer);

    handleSubscriptions();

    return true;
}

void MQTTManager::handleSubscriptions()
{
    if (!client.connected())
        return;

    for (auto &subscription : subscriptions)
    {
        if (!subscription.isActive)
        {
            if (client.subscribe(subscription.topic.c_str()))
            {
                subscription.isActive = true;
            }
        }
    }
}

bool MQTTManager::unsubscribe(const char *topic)
{
    if (!client.connected())
    {
        log.error("MQTTManager", "MQTT client not connected");
        return false;
    }

    if (client.unsubscribe(topic))
    {
        subscriptions.erase(
            std::remove_if(
                subscriptions.begin(),
                subscriptions.end(),
                [topic](const Subscription &subscription)
                {
                    return subscription.topic == topic;
                }),
            subscriptions.end());
        return true;
    }

    return false;
}

bool MQTTManager::publish(const char *subtopic, const char *payload, bool isRetained, bool isAbsoluteTopic)
{
    if (!client.connected())
    {
        log.error("MQTTManager", "MQTT client not connected");
        return false;
    }

    char fullTopic[512];
    if (isAbsoluteTopic)
    {
        snprintf(fullTopic, sizeof(fullTopic), "%s", subtopic);
    }
    else
    {
        snprintf(fullTopic, sizeof(fullTopic), "%s/%s", deviceTopic, subtopic);
    }

    if (client.publish(fullTopic, payload, isRetained))
        return true;

    Serial.println(payload);

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer), "Failed to publish message to topic: %s", fullTopic);
    log.error("MQTTManager", msgBuffer);
    return false;
}

void MQTTManager::update()
{
    if (!initialized)
    {
        return;
    }

    if (!client.connected())
    {
        RuntimeConfig &config = configManager.getRuntimeConfig();
        uint32_t now = millis();

        if (now - lastAttempt >= config.mqtt.retryInterval)
        {
            lastAttempt = now;
            if (connect())
            {
                lastAttempt = 0;
            }
        }
    }
    else
    {
        client.loop();
    }
}

bool MQTTManager::isConnected()
{
    return client.connected();
}