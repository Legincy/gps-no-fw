#include "managers/WifiManager.h"

const char *WifiManager::getWifiStatusString(WiFiStatus status)
{
    switch (status)
    {
    case WiFiStatus::DISCONNECTED:
        return "DISCONNECTED";
    case WiFiStatus::CONNECTING:
        return "CONNECTING";
    case WiFiStatus::CONNECTED:
        return "CONNECTED";
    case WiFiStatus::CONNECTION_FAILED:
        return "CONNECTION_FAILED";
    case WiFiStatus::WRONG_PASSWORD:
        return "WRONG_PASSWORD";
    case WiFiStatus::NO_SSID_AVAILABLE:
        return "NO_SSID_AVAILABLE";
    default:
        return "UNKNOWN";
    }
};

bool WifiManager::begin()
{
    RuntimeConfig &config = configManager.getRuntimeConfig();

    if (strlen(config.wifi.ssid) == 0)
    {
        log.warning("WifiManager", "No SSID available, skipping WifiManager initialization");
        return false;
    }
    else if (strlen(config.wifi.password) == 0)
    {
        log.warning("WifiManager", "No password available, skipping WifiManager initialization");
        return false;
    }

    ftmSemaphore = xSemaphoreCreateBinary();
    WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);

    WiFi.mode(WIFI_STA);
    return true;
}

bool WifiManager::isConnected()
{
    return status == WiFiStatus::CONNECTED && WiFi.status() == WL_CONNECTED;
}

bool WifiManager::connect()
{
    if (status == WiFiStatus::CONNECTED)
    {
        return true;
    }

    RuntimeConfig &config = configManager.getRuntimeConfig();

    char msgBuffer[256];
    snprintf(msgBuffer, sizeof(msgBuffer), "Attempting to connect to Wifi-AP '%s' (['%s', %d], ['%s', %d])", config.wifi.ssid, config.wifi.ssid, strlen(config.wifi.ssid), config.wifi.password, strlen(config.wifi.password));
    log.debug("WifiManager", msgBuffer);

    // WiFi.setMinSecurity(WIFI_AUTH_WEP);
    WiFi.begin(config.wifi.ssid, config.wifi.password);

    status = WiFiStatus::CONNECTING;
    lastAttempt = millis();
    connectionAttempts = 1;

    return true;
}

void WifiManager::disconnect()
{
    log.debug("WifiManager", "Disconnecting from Wifi...");

    WiFi.disconnect();
    status = WiFiStatus::DISCONNECTED;
}

void WifiManager::update()
{
    RuntimeConfig &config = configManager.getRuntimeConfig();
    if (status == WiFiStatus::CONNECTING)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            status = WiFiStatus::CONNECTED;
            connectionAttempts = 0;

            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Connected to Wifi-AP with IP: %s", WiFi.localIP().toString().c_str());
            log.info("WifiManager", buffer);

            return;
        }

        if (millis() - lastAttempt >= config.wifi.checkInterval)
        {
            lastAttempt = millis();
            connectionAttempts++;

            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Connection attempt %d/%d", connectionAttempts, config.wifi.maxConnectionAttempts);
            log.debug("WifiManager", buffer);

            if (connectionAttempts >= config.wifi.maxConnectionAttempts)
            {
                status = WiFiStatus::CONNECTION_FAILED;
                char msgBuffer[192];
                snprintf(msgBuffer, sizeof(msgBuffer), "Failed to connect to Wifi-AP ('%s') due reaching max connection attempts", config.wifi.ssid);
                log.error("WifiManager", msgBuffer);

                return;
            }
        }
    }
    else if (status == WiFiStatus::CONNECTED && WiFi.status() != WL_CONNECTED)
    {
        status = WiFiStatus::DISCONNECTED;
        char msgBuffer[192];
        snprintf(msgBuffer, sizeof(msgBuffer), "Lost connection to Wifi-AP ('%s')", config.wifi.ssid);
        log.warning("WifiManager", msgBuffer);

        if (config.wifi.autoReconnect && millis() - lastAttempt >= config.wifi.reconnectInterval)
        {
            connect();
        }
    }
}

WiFiStatus WifiManager::getStatus()
{
    return status;
}

String WifiManager::getIP()
{
    IPAddress localIP = WiFi.localIP();
    return String(localIP[0]) + "." + String(localIP[1]) + "." + String(localIP[2]) + "." + String(localIP[3]);
}

String WifiManager::getSSID()
{
    return WiFi.SSID();
}

uint8_t *WifiManager::getBSSID()
{
    return WiFi.BSSID();
}

int32_t WifiManager::getRSSI()
{
    return WiFi.RSSI();
}

uint8_t WifiManager::getConnectionAttempts()
{
    return connectionAttempts;
}

bool WifiManager::ftmAP(const char *ssid, const char *password)
{
    return WiFi.softAP(ssid, password, 1, 0, 4, true);
}

void WifiManager::onFtmReport(arduino_event_t *event)
{

    wifi_event_ftm_report_t *report = &event->event_info.wifi_ftm_report;
    WifiManager::getInstance().ftmStatus = report->status;
    WifiManager::getInstance().ftmDistance = report->dist_est;

    /*char msgBuffer[256];
    snprintf(msgBuffer, sizeof(msgBuffer), "FTM Report: Status: %s, Distance: %d", WifiManager::getInstance().ftm_status_str[report->status], report->dist_est);
    WifiManager::getInstance().log.debug("WifiManager", msgBuffer);*/

    free(report->ftm_report_data);
    xSemaphoreGive(WifiManager::getInstance().ftmSemaphore);
}

bool WifiManager::initiateFtm(uint8_t channel, byte mac[])
{

    RuntimeConfig &config = configManager.getRuntimeConfig();

    if (!WiFi.initiateFTM(config.wifi.ftmFrameCount, config.wifi.ftmBurstPeriod, channel, mac))
    {
        log.error("WifiManager", "Failed to initiate FTM session");
        return false;
    }

    return xSemaphoreTake(ftmSemaphore, portMAX_DELAY) == pdPASS && this->ftmStatus == FTM_STATUS_SUCCESS;
}

int WifiManager::scan(bool ftm = false)
{
    int n = WiFi.scanNetworks();

    if (n == 0)
    {
        log.warning("WifiManager", "No networks found");
    }
    else
    {
        char msgBuffer[256];
        snprintf(msgBuffer, sizeof(msgBuffer), "Found %d networks", n);
        log.info("WifiManager", msgBuffer);

        Serial.printf("| SSID                             | RSSI | CH | MAC               | FTM Status    | Distance |\n");
        Serial.printf("|----------------------------------|------|----|-------------------|---------------|----------|\n");

        for (int i = 0; i < n; ++i)
        {

            if (ftm)
            {
                initiateFtm(WiFi.channel(i), WiFi.BSSID(i));
            }

            Serial.printf("| %-32.32s | %4ld | %2ld | %02X:%02X:%02X:%02X:%02X:%02X | %13s | %8.2f |\n",
                          WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i),
                          WiFi.BSSID(i)[0], WiFi.BSSID(i)[1], WiFi.BSSID(i)[2], WiFi.BSSID(i)[3], WiFi.BSSID(i)[4], WiFi.BSSID(i)[5],
                          ftm ? ftm_status_str[ftmStatus] : "", (float)ftmDistance / 100.0);
        }
    }

    WiFi.scanDelete();
    return n;
}