#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include "ConfigManager.h"

enum class LogLevel
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    __DELIMITER__
};

class LogManager
{
private:
    LogManager()
        : configManager(ConfigManager::getInstance()), deviceId(configManager.getRuntimeConfig().device.name), logLevel(static_cast<LogLevel>(configManager.getRuntimeConfig().logging.logLevel))
    {
    }

    PubSubClient *mqttClient;
    static const char *LOG_TOPIC_PREFIX;
    ConfigManager &configManager;
    const char *deviceId;
    LogLevel logLevel;

    const char *getLogLevelString(LogLevel level);
    bool isLogLevelEnabled(LogLevel level);
    constexpr size_t getLogLevelCount() { return static_cast<size_t>(LogLevel::__DELIMITER__); };

public:
    LogManager(const LogManager &) = delete;
    void operator=(const LogManager &) = delete;

    static LogManager &getInstance()
    {
        static LogManager instance;
        return instance;
    }

    void log(LogLevel level, const char *source, const char *message);
    void info(const char *source, const char *message);
    void warning(const char *source, const char *message);
    void error(const char *source, const char *message);
    void debug(const char *source, const char *message);
    void setLogLevel(LogLevel level) { logLevel = level; };

    LogLevel getLogLevel() { return logLevel; };
    const char *getLogLevelString() { return getLogLevelString(logLevel); };
};

#endif
