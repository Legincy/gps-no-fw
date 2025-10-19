#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "LogManager.h"
#include "ConfigDefines.h"
#include "ConfigManager.h"

enum class WiFiStatus
{
    UNINITIALIZED,
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONNECTION_FAILED,
    WRONG_PASSWORD,
    NO_SSID_AVAILABLE,
    __DELIMITER__
};

class WifiManager
{
private:
    WifiManager()
        : logManager(LogManager::getInstance()), configManager(ConfigManager::getInstance()), runtimeconfig(configManager.getRuntimeConfig()) {}
    LogManager &logManager;
    ConfigManager &configManager;
    RuntimeConfig &runtimeconfig;

public:
    WifiManager(const WifiManager &) = delete;
    static WifiManager &getInstance()
    {
        static WifiManager instance;
        return instance;
    }

    /**
     * @brief Sets the WiFi module to station mode and disconnects any existing connection.
     *
     * This method configures the WiFi module by setting it to station mode via WiFi.mode(WIFI_STA)
     * and then disconnects any current connection using WiFi.disconnect(). It returns the logical
     * AND of both function calls, indicating success only if both operations return true.
     *
     * @return true if both setting the mode and disconnecting are successful, false otherwise.
     */
    bool begin();

    /**
     * @brief Attempts to connect to the configured WiFi network.
     *
     * This method performs several checks and steps:
     * - It verifies that the configured SSID is non-empty and is found in the scan results using isTargetSSIDFound().
     *   If the SSID is empty or not found, an error is logged and the method returns false.
     * - It verifies that a WiFi password is provided. If not, an error is logged and false is returned.
     * - It attempts to begin the WiFi connection using WiFi.begin() with the provided SSID and password.
     *   If this call fails, an error is logged and the method returns false.
     * - It waits up to 60 seconds for the WiFi status to become WL_CONNECTED.
     *   If the connection is not established within this timeout, an error is logged (suggesting a potential wrong password)
     *   and the method returns false.
     * - Once connected, it registers the WiFi event handler (handleWiFiEvent) to handle future WiFi events.
     *
     * @return true if the connection is successfully established, otherwise false.
     */
    bool connect();

    /**
     * @brief Extended WiFi event handler to handle various WiFi events.
     *
     * This static callback function is invoked when a WiFi event occurs.
     * It handles multiple events, including:
     * - SYSTEM_EVENT_WIFI_READY: WiFi interface is ready.
     * - SYSTEM_EVENT_SCAN_DONE: Scanning for access points is finished.
     * - SYSTEM_EVENT_STA_START: Station mode has started.
     * - SYSTEM_EVENT_STA_STOP: Station mode has stopped.
     * - SYSTEM_EVENT_STA_CONNECTED: Station connected to an access point.
     * - SYSTEM_EVENT_STA_DISCONNECTED: Station disconnected from an access point.
     * - SYSTEM_EVENT_STA_AUTHMODE_CHANGE: Authentication mode of the connected AP changed.
     * - SYSTEM_EVENT_STA_GOT_IP: Station obtained an IP address.
     * - SYSTEM_EVENT_STA_LOST_IP: Station lost its IP address.
     *
     * For each event, an appropriate log message is generated.
     * In the case of SYSTEM_EVENT_STA_DISCONNECTED, a reconnection attempt is triggered.
     *
     * @param event The WiFi event that occurred.
     */
    static void handleWiFiEvent(WiFiEvent_t event);

    /**
     * @brief Checks if the given target SSID is present in the WiFi scan results.
     *
     * This method performs a WiFi scan and iterates through the found networks.
     * If the provided target SSID is found, it returns true; otherwise, it logs a warning
     * and returns false.
     *
     * @param targetSSID The SSID to search for in the scan results.
     * @return true if the target SSID is found, false otherwise.
     */
    bool isTargetSSIDFound(const char *targetSSID);
};

#endif