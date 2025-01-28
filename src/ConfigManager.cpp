#include "ConfigManager.h"

void ConfigManager::loadDefaults() {
    setConfigFromDefines(&config);

    config.device.chipID = ESP.getEfuseMac();

    String mac = WiFi.macAddress();
    mac.toCharArray(config.device.macAddress, sizeof(config.device.macAddress));
    config.device.macAddress[sizeof(config.device.macAddress) - 1] = '\0';

    String hash = calculateHash(&config);
    strncpy(config.hash, hash.c_str(), sizeof(config.hash) - 1);
    config.hash[sizeof(config.hash) - 1] = '\0';
}

void ConfigManager::print(RuntimeConfig* config) {
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
    Serial.printf("Config Hash: %s\n", config->hash);
}

String ConfigManager::calculateHash(RuntimeConfig* config) {
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

    return md5.toString();
}

bool ConfigManager::begin() {
    if (initialized) {
        return true;
    }

    if (ESP.getFreeHeap() < 10000) {
        Serial.println(F("Warning: Low memory"));
    }

    if(!LittleFS.begin(true)) {
        Serial.println(F("Failed to mount file system"));
        loadDefaults();
        return false;
    }

    if (LittleFS.totalBytes() - LittleFS.usedBytes() < sizeof(RuntimeConfig)) {
        Serial.println(F("Warning: Low storage space"));
    }

    if(!DEBUG_FORCE_CONFIG){
        if(!loadFromFlash()) {
            Serial.println(F("Failed to load config from flash, using defaults"));        
            if (!saveToFlash()) {
                Serial.println(F("Failed to save defaults to flash"));
                return false;
            }
        }
    }

    initialized = true;
    return true;
}

void ConfigManager::setConfigFromDefines(RuntimeConfig* config) {
    memset(config, 0, sizeof(RuntimeConfig));

    /* Helper-Makro zur sicheren String-Kopie */
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

    /* Makro entfernen, um Seiteneffekte zu vermeiden */
    #undef SAFE_STRLCPY
}

bool ConfigManager::loadFromFlash() {
    File file = LittleFS.open(CONFIG_FILE, "r");
    if(!file || file.size() != sizeof(RuntimeConfig)) {
        if (file) file.close();
        return false;
    }

    if (file.readBytes((char*)&config, sizeof(RuntimeConfig)) != sizeof(RuntimeConfig)) {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool ConfigManager::saveToFlash() {
    File file = LittleFS.open(CONFIG_FILE, "w");
    if(!file) {
        return false;
    }

    if (file.write((const uint8_t*)&config, sizeof(RuntimeConfig)) != sizeof(RuntimeConfig)) {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool ConfigManager::hasConfigDefinesChanged() {
    RuntimeConfig defaultConfig;
    setConfigFromDefines(&defaultConfig);

    String defaultConfigHash = calculateHash(&defaultConfig);
    Serial.printf("File Config Hash: %s - Stored Config Hash: %s\n", defaultConfigHash.c_str(), config.hash);
    return strcmp(defaultConfigHash.c_str(), config.hash) != 0;
}

void ConfigManager::updateDeviceConfig() {
    loadDefaults();
    saveToFlash();
    Serial.println(F("Updated config"));
}