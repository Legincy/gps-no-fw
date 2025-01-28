#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <map>
#include <vector>
#include "ConfigManager.h"
#include "Logger.h"

typedef std::function<void(char*, uint8_t*, unsigned int)> MQTTCallback;

struct Subscription {
    String topic;
    MQTTCallback callback;
};

class MQTTManager {
private:
    MQTTManager() 
        : client(espClient)
        , initialized(false)
        , lastAttempt(0)
        , log(Logger::getInstance())
        , configManager(ConfigManager::getInstance()) {
             
        client.setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
            this->handleCallback(topic, payload, length);
        });
    }

    WiFiClient espClient;
    PubSubClient client;
    bool initialized;
    uint32_t lastAttempt;
    uint8_t connectionAttemts;
    String clientId;
    std::vector<Subscription> subscriptions;
    Logger& log;
    ConfigManager& configManager;

    void handleCallback(char* topic, uint8_t* payload, uint32_t length);
    bool matchTopic(const char* pattern, const char* topic);

public:
    MQTTManager(const MQTTManager&) = delete;
    void operator=(const MQTTManager&) = delete;

    static MQTTManager& getInstance() {
        static MQTTManager instance;
        return instance;
    }

    bool begin();
    bool connect();
    void disconnect();
    bool subscribe(const char* topic, MQTTCallback callback);
    bool unsubscribe(const char* topic);
    bool publish(const char* topic, const char* payload, bool retained = false);
    void update();
    bool isConnected();

    PubSubClient& getClient() { return client; }
    String getClientId() { return clientId; }
};

#endif
