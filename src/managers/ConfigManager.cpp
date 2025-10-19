#include "managers/ConfigManager.h"

void ConfigManager::loadDefaults()
{
    setConfigFromDefines(&config);

    config.device.chipID = ESP.getEfuseMac();

    String mac = WiFi.macAddress();
    mac.toCharArray(config.device.macAddress, sizeof(config.device.macAddress));
    config.device.macAddress[sizeof(config.device.macAddress) - 1] = '\0';

    String modifiedMac = mac;
    modifiedMac.replace(":", "");

    snprintf(config.bluetooth.serviceUUID, sizeof(config.bluetooth.serviceUUID),
             "%s-0000-5000-8000-000000000000",
             modifiedMac.c_str());

    snprintf(config.bluetooth.charUUID, sizeof(config.bluetooth.charUUID),
             "%s-0000-5000-8000-000000000001",
             modifiedMac.c_str());

    char hashBuffer[ConfigLimits::CONFIG_HASH_MAX_LENGTH];
    calculateHash(&config, hashBuffer, sizeof(hashBuffer));

    strncpy(config.hash, hashBuffer, sizeof(hashBuffer));
    config.hash[sizeof(config.hash) - 1] = '\0';
}

void ConfigManager::print(RuntimeConfig *config)
{
    Serial.printf("Device Name: %s\n", config->device.name);
    Serial.printf("Firmware Version: %s\n", config->device.firmwareVersion);
    Serial.printf("WiFi SSID: %s\n", config->wifi.ssid);
    Serial.printf("WiFi Password: %s\n", config->wifi.password);
    Serial.printf("WiFi Auto Reconnect: %s\n", config->wifi.autoReconnect ? "true" : "false");
    Serial.printf("WiFi Check Interval: %d\n", config->wifi.checkInterval);
    Serial.printf("WiFi Reconnect Interval: %d\n", config->wifi.reconnectInterval);
    Serial.printf("WiFi Max Connection Attempts: %d\n", config->wifi.maxConnectionAttempts);
    Serial.printf("MQTT Broker: %s\n", config->mqtt.broker);
    Serial.printf("MQTT Port: %d\n", config->mqtt.port);
    Serial.printf("MQTT User: %s\n", config->mqtt.user);
    Serial.printf("MQTT Password: %s\n", config->mqtt.password);
    Serial.printf("MQTT Retry Interval: %d\n", config->mqtt.retryInterval);
    Serial.printf("MQTT Base Topic: %s\n", config->mqtt.baseTopic);
    Serial.printf("Chip ID: %llu\n", config->device.chipID);
    Serial.printf("MAC Address: %s\n", config->device.macAddress);
    Serial.printf("Error Max Recovery Attempts: %d\n", config->error.maxRecoveryAttempts);
    Serial.printf("Error Recovery Interval: %d\n", config->error.recoveryInterval);
    Serial.printf("Logging Allow MQTT Log: %s\n", config->logging.allowMqttLog ? "true" : "false");
    Serial.printf("Logging MQTT Topic: %s\n", config->logging.mqttTopic);
    Serial.printf("Logging Level: %d\n", config->logging.logLevel);
    Serial.printf("Update API URL: %s\n", config->update.apiUrl);
    Serial.printf("Update API Token: %s\n", config->update.apiToken);
    Serial.printf("Update Interval: %d\n", config->update.interval);
    Serial.printf("Update Initial Check: %s\n", config->update.initialCheck ? "true" : "false");
    Serial.printf("Bluetooth Service UUID: %s\n", config->bluetooth.serviceUUID);
    Serial.printf("Bluetooth Char UUID: %s\n", config->bluetooth.charUUID);
    Serial.printf("Bluetooth Timeout: %d\n", config->bluetooth.timeout);
    Serial.printf("Bluetooth Max Connections: %d\n", config->bluetooth.maxConnections);
    Serial.printf("Config Hash: %s\n", config->hash);
}

bool ConfigManager::validateConfig(const RuntimeConfig *pConfig, char *buffer, size_t bufferSize)
{
    /*
    TODO: Validation check if config parameters are valid (e.g. not empty, too long, etc.)

    snprintf(buffer, bufferSize, "device.statusUpdatesInterval has to be greater than 0");
    if (pConfig->device.statusUpdateInterval <= 0) return false;
    */

    return true;
}

void ConfigManager::calculateHash(RuntimeConfig *config, char *hashBuffer, size_t hashBufferSize)
{
    MD5Builder md5;
    md5.begin();

    String configString =
        String(config->device.name) +
        String(config->wifi.ssid) +
        String(config->wifi.password) +
        String(config->mqtt.broker) +
        String(config->mqtt.port) +
        String(config->mqtt.user) +
        String(config->mqtt.password) +
        String(config->mqtt.retryInterval);

    md5.add(configString);
    md5.calculate();

    md5.toString().toCharArray(hashBuffer, sizeof(hashBuffer));
}

bool ConfigManager::begin()
{
    if (initialized)
        return true;

    if (ESP.getFreeHeap() < 10000)
    {
        Serial.println("Free Heap: " + String(ESP.getFreeHeap()));
    }

    if (!LittleFS.begin(true))
    {
        Serial.println("Failed to mount file system");
        loadDefaults();

        return false;
    }

    if (LittleFS.totalBytes() - LittleFS.usedBytes() < sizeof(RuntimeConfig))
    {
        Serial.println(F("Warning: Low storage space"));
    }

    if (!DEBUG_FORCE_CONFIG)
    {
        if (!loadFromFlash())
        {
            Serial.println(F("Failed to load config from flash, using defaults"));
            if (!saveToFlash())
            {
                Serial.println(F("Failed to save defaults to flash"));
                return false;
            }
        }
    }

    initialized = true;
    return true;
}

void ConfigManager::setConfigFromDefines(RuntimeConfig *config)
{
    memset(config, 0, sizeof(RuntimeConfig));

#define SAFE_STRLCPY(dest, src) strlcpy(dest, src, sizeof(dest))

    /* #### DEVICE #### */
    SAFE_STRLCPY(config->device.name, DEVICE_NAME);
    config->device.statusUpdateInterval = DEVICE_HEARTBEAT_INTERVAL;

    /* #### WIFI #### */
    SAFE_STRLCPY(config->wifi.ssid, WIFI_SSID);
    SAFE_STRLCPY(config->wifi.password, WIFI_PASSWORD);
    config->wifi.autoReconnect = WIFI_AUTO_RECONNECT;
    config->wifi.checkInterval = WIFI_CHECK_INTERVAL;
    config->wifi.reconnectInterval = WIFI_RECONNECT_INTERVAL;
    config->wifi.maxConnectionAttempts = WIFI_MAX_CONNECTION_ATTEMPTS;
    config->wifi.ftmFrameCount = WIFI_FTM_FRAME_COUNT;
    config->wifi.ftmBurstPeriod = WIFI_FTM_BURST_PERIOD;

    /* #### MQTT #### */
    SAFE_STRLCPY(config->mqtt.broker, MQTT_BROKER);
    config->mqtt.port = MQTT_PORT;
    SAFE_STRLCPY(config->mqtt.user, MQTT_USER);
    SAFE_STRLCPY(config->mqtt.password, MQTT_PASSWORD);
    config->mqtt.retryInterval = MQTT_RETRY_INTERVAL;
    config->mqtt.maxConnectionAttempts = MQTT_MAX_CONNECTION_ATTEMPTS;
    SAFE_STRLCPY(config->mqtt.baseTopic, MQTT_BASE_TOPIC);

    /* #### ERROR #### */
    config->error.maxRecoveryAttempts = ERROR_MAX_RECOVERY_ATTEMPTS;
    config->error.recoveryInterval = ERROR_RECOVERY_INTERVAL;

    /* #### LOGGING #### */
    config->logging.allowMqttLog = LOGGING_ALLOW_MQTT_LOG;
    config->logging.logLevel = LOGGING_LEVEL;
    SAFE_STRLCPY(config->logging.mqttTopic, LOGGING_MQTT_TOPIC);

    /* #### BLUETOOTH #### */
    config->bluetooth.timeout = BLUETOOTH_TIMEOUT;
    config->bluetooth.maxConnections = BLUETOOTH_MAX_CONNECTIONS;

    /* #### UPDATE #### */
    SAFE_STRLCPY(config->update.apiUrl, UPDATE_GITHUB_API_URL);
    SAFE_STRLCPY(config->update.apiToken, UPDATE_GITHUB_API_TOKEN);
    config->update.interval = UPDATE_INTERVAL;
    config->update.initialCheck = UPDATE_INITIAL_CHECK;

#undef SAFE_STRLCPY
}

bool ConfigManager::loadFromFlash()
{
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file || file.size() != sizeof(RuntimeConfig))
    {
        if (file)
            file.close();
        return false;
    }

    if (file.readBytes((char *)&config, sizeof(RuntimeConfig)) != sizeof(RuntimeConfig))
    {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool ConfigManager::saveToFlash()
{
    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file)
    {
        return false;
    }

    if (file.write((const uint8_t *)&config, sizeof(RuntimeConfig)) != sizeof(RuntimeConfig))
    {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool ConfigManager::hasConfigDefinesChanged()
{
    RuntimeConfig defaultConfig;
    setConfigFromDefines(&defaultConfig);

    char hashBuffer[ConfigLimits::CONFIG_HASH_MAX_LENGTH];
    calculateHash(&defaultConfig, hashBuffer, sizeof(hashBuffer));

    return strcmp(hashBuffer, config.hash) != 0;
}

void ConfigManager::updateDeviceConfig()
{
    loadDefaults();
    saveToFlash();
    Serial.println(F("Updated config"));
}