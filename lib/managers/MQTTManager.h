#ifndef ESP_MQTT_CLIENT_H
#define ESP_MQTT_CLIENT_H

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ConfigManager.h>
#include <vector>
#include <functional>
#include "LogManager.h"
#include "ConfigDefines.h"
#include <esp_system.h>

// Callback type for received MQTT messages
typedef std::function<void(char *, uint8_t *, unsigned int)> MQTTCallback;

// Struct representing a subscription record.
struct Subscription
{
    String topic;
    MQTTCallback callback;
};

class MQTTManager
{
private:
    MQTTManager();
    LogManager &logManager;       // Logging instance
    ConfigManager &configManager; // Configuration manager instance
    RuntimeConfig &runtimeConfig;
    WiFiClient _wifiClient;   // WiFi client
    PubSubClient _mqttClient; // MQTT client
    // bool _initialized;
    uint16_t _qos;
    uint16_t _keepAlive;
    char _clientId[64];
    char _pubTopic[64];
    char _subTopic[64];
    char _devTopic[64]; // send log to mqtt broker (not implemented)
    // List of topic subscriptions
    std::vector<Subscription> _subscriptions;
    // Private callback for incoming MQTT messages.
    static void handleCallback(char *topic, byte *payload, unsigned int length);

public:
    // Kopierkonstruktor und Zuweisungsoperator löschen (Singleton)
    MQTTManager(const MQTTManager &) = delete;
    MQTTManager &operator=(const MQTTManager &) = delete;

    /**
     * @brief Returns the singleton instance of MQTTManager.
     *
     * Make sure to call an appropriate constructor beforehand or adapt this method
     * to initialize the instance as needed.
     *
     * @return The singleton instance.
     */

    static MQTTManager &getInstance()
    {
        static MQTTManager instance;
        return instance;
    }

    /**
     * @brief Initializes the MQTT client.
     *
     * Configures the MQTT server, buffer size, and callback function.
     * Logs an error if the MQTT broker is not defined.
     *
     * @return true if initialization is successful, false otherwise.
     */
    bool begin();

    /**
     * @brief Connects to the MQTT broker.
     *
     * Attempts to establish a connection to the MQTT broker using the configured credentials.
     *
     * @return true if the connection is successful, false otherwise.
     */
    void connect();

    /**
     * @brief Disconnects from the MQTT broker.
     */
    void disconnect();

    /**
     * @brief Publishes a message to a given MQTT topic.
     *
     * If isAbsoluteTopic is false, the topic is prefixed with the device topic.
     *
     * @param topic The (sub-)topic to publish to.
     * @param payload The message payload.
     * @param retain Whether the message should be retained.
     * @param isAbsoluteTopic Whether the topic is absolute.
     * @return true if the message was published successfully, false otherwise.
     */
    bool publish(const char *topic, const char *payload, bool retain = false, bool isAbsoluteTopic = false);

    /**
     * @brief Subscribes to a given MQTT topic.
     *
     * Registers a callback to be executed when messages on the topic are received.
     *
     * @param topic The topic to subscribe to.
     * @param callback The callback function.
     * @return true if the subscription is successful, false otherwise.
     */
    bool subscribe(const char *topic, MQTTCallback callback);

    /**
     * @brief Processes incoming MQTT messages and maintains the connection.
     *
     * This method should be called regularly in the main loop.
     */
    void update();
    bool publishMeasurement(const char *payload);
    bool publishConfig(const char *payload);
};

#endif
