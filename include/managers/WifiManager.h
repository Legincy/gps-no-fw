#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "ConfigManager.h"
#include "LogManager.h"

enum class WiFiStatus {
    UNINITIALIZED,
    DISCONNECTED,
    CONNECTING,
    CONNECTED, 
    CONNECTION_FAILED,
    WRONG_PASSWORD,
    NO_SSID_AVAILABLE,
    __DELIMITER__
};

class WifiManager {
private:
    WifiManager()
        : status(WiFiStatus::DISCONNECTED)
        , lastAttempt(0)
        , connectionAttempts(0)
        , configManager(ConfigManager::getInstance())
        , log(LogManager::getInstance()) {}

    WiFiStatus status;
    uint32_t lastAttempt;
    uint8_t connectionAttempts;

    ConfigManager& configManager;
    LogManager& log;

    SemaphoreHandle_t ftmSemaphore;
    wifi_ftm_status_t ftmStatus;
    uint32_t ftmDistance;

    const char *getWifiStatusString(WiFiStatus status);
    constexpr size_t getWifiStatusCount() {return static_cast<size_t>(WiFiStatus::__DELIMITER__);};

    static void onFtmReport(arduino_event_t *event);
    const char *getWifiFtmStatusString(wifi_ftm_status_t status);

public:
    WifiManager(const WifiManager&) = delete;
    void operator=(const WifiManager&) = delete;

    static WifiManager& getInstance() {
        static WifiManager instance;
        return instance;
    }

    bool begin();
    void update();
    bool connect();
    void disconnect();
    void reconnect();
    void printStatus();
    bool isConnected();
    void setAutoReconnect(bool isEnabled);
    
    String getIP();
    String getSSID();
    uint8_t* getBSSID();
    int32_t getRSSI();
    uint8_t getConnectionAttempts();

    WiFiStatus getStatus();
    const char* getStatusString() { return getWifiStatusString(status); };
    
    wifi_ftm_status_t getFtmStatus();
    const char* getFtmStatusString() { return getWifiFtmStatusString(ftmStatus); };
    uint32_t getFtmDistance();

    bool startAP(const char* ssid, const char* password, uint8_t channel = 1, bool hidden = false, int maxConnections = 4, bool ftm = true);
    bool initiateFtm(uint8_t channel, byte mac[]);
    int scan(bool ftm);
};

#endif