#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <map>
#include <vector>
#include "ConfigManager.h"
#include "LogManager.h"

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
        : client(espClient), initialized(false), lastAttempt(0), log(LogManager::getInstance()), configManager(ConfigManager::getInstance())
    {
        RuntimeConfig &config = configManager.getRuntimeConfig();
        uint64_t chipId = config.device.chipID;
        uint8_t mac[6] = {
            (uint8_t)((chipId >> 40) & 0xFF),
            (uint8_t)((chipId >> 32) & 0xFF),
            (uint8_t)((chipId >> 24) & 0xFF),
            (uint8_t)((chipId >> 16) & 0xFF),
            (uint8_t)((chipId >> 8) & 0xFF),
            (uint8_t)(chipId & 0xFF)};
        char macStr[13]; // 12 Zeichen + '\0'
        snprintf(macStr, sizeof(macStr),
                 "%02x%02x%02x%02x%02x%02x",
                 mac[0], mac[1], mac[2],
                 mac[3], mac[4], mac[5]);
        snprintf(deviceTopic, sizeof(deviceTopic),
                 "%s/%s",
                 config.mqtt.baseTopic,
                 macStr);
        snprintf(clientId, sizeof(clientId),
                 "%s-%s",
                 config.device.name,
                 macStr);
        }

    LogManager &log;
    ConfigManager &configManager;

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
