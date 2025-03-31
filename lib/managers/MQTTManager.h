#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <map>
#include <vector>
#include "LogManager.h"
#include "ConfigDefines.h"

typedef std::function<void(char *, uint8_t *, unsigned int)> MQTTCallback;

struct Subscription
{
    String topic;
    MQTTCallback callback;
    bool isPersistent;
    bool isActive;
};

class MQTTManager
{
private:
    MQTTManager()
        : client(espClient), initialized(false), lastAttempt(0), log(LogManager::getInstance())
    {
        snprintf(deviceTopic, sizeof(deviceTopic), "%s/%u", MQTT_BASE_TOPIC, 123456);
        snprintf(clientId, sizeof(clientId), "%s-%x", DEVICE_NAME, 123456);
    }

    LogManager &log;

    WiFiClient espClient;
    PubSubClient client;
    bool initialized;
    uint32_t lastAttempt;
    uint8_t connectionAttempts;
    std::vector<Subscription> subscriptions;
    char deviceTopic[128];
    char clientId[64];

    void handleSubscriptions();
    void handleCallback(char *topic, uint8_t *payload, uint32_t length);
    bool matchTopic(const char *pattern, const char *topic);

public:
    MQTTManager(const MQTTManager &) = delete;
    void operator=(const MQTTManager &) = delete;

    static MQTTManager &getInstance()
    {
        static MQTTManager instance;
        return instance;
    }

    bool begin();
    bool connect();
    void disconnect();
    bool subscribe(const char *topic, MQTTCallback callback, bool isPersistent = false);
    bool unsubscribe(const char *topic);
    bool publish(const char *topic, const char *payload, bool isRetained = false, bool isAbsoluteTopic = false);
    void update();
    bool isConnected();
    bool isSubscribed(const char *topic);

    PubSubClient &getClient() { return client; }
    const char *getClientId() { return clientId; }
    const char *getDeviceTopic() { return deviceTopic; }
};

#endif
