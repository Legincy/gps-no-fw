#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <map>
#include <vector>
#include "ConfigManager.h"
#include "LogManager.h"
// #include "UWBManager.h"

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
        : client(espClient), initialized(false), lastAttempt(0), log(LogManager::getInstance()), configManager(ConfigManager::getInstance()) //, uwbManager(UWBManager::getInstance())
    {
        RuntimeConfig &config = configManager.getRuntimeConfig();
        snprintf(stationTopic, sizeof(stationTopic),
                 "%s/stations/%s",
                 config.mqtt.baseTopic,
                 config.device.modifiedMac);
        snprintf(clientId, sizeof(clientId),
                 "%s-%s",
                 config.device.name,
                 config.device.modifiedMac);
    }

    LogManager &log;
    ConfigManager &configManager;
    // UWBManager &uwbManager;
    WiFiClient espClient;
    PubSubClient client;
    bool initialized;
    uint32_t lastAttempt;
    uint8_t connectionAttempts;
    std::vector<Subscription> subscriptions;
    char stationTopic[128];
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
    bool publishMeasurement(const char *payload);
    void update();
    bool isConnected();
    bool isSubscribed(const char *topic);

    PubSubClient &getClient() { return client; }
    const char *getClientId() { return clientId; }
    const char *getStationTopic() { return stationTopic; }
};

#endif
