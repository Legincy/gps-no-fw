#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <MD5Builder.h>
#include "WiFi.h"
#include "ConfigDefines.h"

namespace ConfigLimits
{
    constexpr size_t CONFIG_DEVICE_NAME_MAX_LENGTH = 32;
    constexpr size_t CONFIG_DEVICE_FIRMWARE_VERSION_MAX_LENGTH = 16;
    constexpr size_t CONFIG_DEVICE_MAC_ADDRESS_MAX_LENGTH = 18;
    constexpr size_t CONFIG_WIFI_SSID_MAX_LENGTH = 33;
    constexpr size_t CONFIG_WIFI_PASSWORD_MAX_LENGTH = 64;
    constexpr size_t CONFIG_MQTT_BROKER_MAX_LENGTH = 64;
    constexpr size_t CONFIG_MQTT_USER_MAX_LENGTH = 32;
    constexpr size_t CONFIG_MQTT_PASSWORD_MAX_LENGTH = 64;
    constexpr size_t CONFIG_MQTT_BASE_TOPIC_MAX_LENGTH = 64;
    constexpr size_t CONFIG_LOGGING_MQTT_TOPIC_MAX_LENGTH = 64;
    constexpr size_t CONFIG_UPDATE_API_URL_MAX_LENGTH = 128;
    constexpr size_t CONFIG_UPDATE_API_TOKEN_MAX_LENGTH = 128;
    constexpr size_t CONFIG_BLUETOOTH_SERVICE_UUID_MAX_LENGTH = 37;
    constexpr size_t CONFIG_BLUETOOTH_CHAR_UUID_MAX_LENGTH = 37;
    constexpr size_t CONFIG_HASH_MAX_LENGTH = 33;
}

struct RuntimeConfig
{
    struct
    {
        char name[ConfigLimits::CONFIG_DEVICE_NAME_MAX_LENGTH];
        char firmwareVersion[ConfigLimits::CONFIG_DEVICE_FIRMWARE_VERSION_MAX_LENGTH];
        uint64_t chipID;
        char macAddress[ConfigLimits::CONFIG_DEVICE_MAC_ADDRESS_MAX_LENGTH];
        uint32_t statusUpdateInterval;
    } device;

    struct
    {
        char ssid[ConfigLimits::CONFIG_WIFI_SSID_MAX_LENGTH];
        char password[ConfigLimits::CONFIG_WIFI_PASSWORD_MAX_LENGTH];
        bool autoReconnect;
        uint8_t maxConnectionAttempts;
        uint32_t reconnectInterval;
        uint32_t checkInterval;
        uint8_t ftmFrameCount = 16;
        uint16_t ftmBurstPeriod = 2;
    } wifi;

    struct
    {
        char broker[ConfigLimits::CONFIG_MQTT_BROKER_MAX_LENGTH];
        uint16_t port;
        char user[ConfigLimits::CONFIG_MQTT_USER_MAX_LENGTH];
        char password[ConfigLimits::CONFIG_MQTT_PASSWORD_MAX_LENGTH];
        uint32_t retryInterval;
        char baseTopic[ConfigLimits::CONFIG_MQTT_BASE_TOPIC_MAX_LENGTH];
        uint8_t maxConnectionAttempts;
    } mqtt;

    struct
    {
        char serviceUUID[ConfigLimits::CONFIG_BLUETOOTH_SERVICE_UUID_MAX_LENGTH];
        char charUUID[ConfigLimits::CONFIG_BLUETOOTH_CHAR_UUID_MAX_LENGTH];
        uint32_t timeout;
        uint8_t maxConnections;
    } bluetooth;

    struct
    {
        uint8_t maxRecoveryAttempts;
        uint32_t recoveryInterval;
    } error;

    struct
    {
        bool allowMqttLog;
        char mqttTopic[ConfigLimits::CONFIG_LOGGING_MQTT_TOPIC_MAX_LENGTH];
        uint8_t logLevel;
    } logging;

    struct
    {
        char apiUrl[ConfigLimits::CONFIG_UPDATE_API_URL_MAX_LENGTH];
        char apiToken[ConfigLimits::CONFIG_UPDATE_API_TOKEN_MAX_LENGTH];
        uint32_t interval;
        bool initialCheck;
    } update;

    char hash[ConfigLimits::CONFIG_HASH_MAX_LENGTH];
};

enum class ConfigError
{
    NONE,
    FLASH_MOUNT_FAILED,
    FILE_NOT_FOUND,
    READ_ERROR,
    WRITE_ERROR,
    VALIDATION_ERROR
};

class ConfigManager
{
private:
    ConfigManager() : initialized(false)
    {
        loadDefaults();
    }

    RuntimeConfig config;
    static constexpr const char *CONFIG_FILE = "/config.bin";
    bool initialized;

    void calculateHash(RuntimeConfig *config, char *hashBuffer, size_t hashBufferSize);
    bool validateConfig(const RuntimeConfig *pConfig, char *buffer, size_t bufferSize);
    bool loadFromFlash();
    bool saveToFlash();
    void loadDefaults();
    void setConfigFromDefines(RuntimeConfig *config);

public:
    ConfigManager(const ConfigManager &) = delete;
    void operator=(const ConfigManager &) = delete;

    static ConfigManager &getInstance()
    {
        static ConfigManager instance;
        return instance;
    }
    bool begin();
    RuntimeConfig &getRuntimeConfig() { return config; }
    bool hasConfigDefinesChanged();
    void updateDeviceConfig();
    void print(RuntimeConfig *config);
};

#endif