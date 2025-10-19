#ifndef ERROR_CODES_H
#define ERROR_CODES_H

enum class ErrorCode
{
    // System/Boot Errors (1-9)
    BOOT_FAILED = 1,
    MEMORY_LOW = 2,
    FILESYSTEM_ERROR = 3,

    // Configuration Errors (10-19)
    CONFIG_INVALID = 10,
    CONFIG_LOAD_FAILED = 11,
    CONFIG_SAVE_FAILED = 12,

    // WiFi Errors (20-29)
    WIFI_INIT_FAILED = 20,
    WIFI_CONNECTION_FAILED = 21,
    WIFI_CONNECTION_LOST = 22,
    WIFI_WRONG_PASSWORD = 23,
    WIFI_NO_SSID = 24,

    // MQTT Errors (30-39)
    MQTT_INIT_FAILED = 30,
    MQTT_CONNECTION_FAILED = 31,
    MQTT_LOST_CONNECTION = 32,
    MQTT_SUBSCRIPTION_FAILED = 33,
    MQTT_PUBLISH_FAILED = 34,

    // Sensor/Hardware Errors (40-49)
    SENSOR_INIT_FAILED = 40,
    SENSOR_READ_FAILED = 41,
    SENSOR_CALIBRATION_FAILED = 42,

    // Recovery/Update Errors (50-59)
    RECOVERY_FAILED = 50,
    UPDATE_FAILED = 51,

    // Bluetooth Errors (60-69)
    BLUETOOTH_INIT_FAILED = 60,
    BLUETOOTH_SERVER_FAILED = 61,
    BLUETOOTH_CLIENT_FAILED = 62,

    UNKNOWN_ERROR = 99,
    __DELIMITER__
};

namespace ErrorUtils
{
    inline bool isSystemError(ErrorCode code)
    {
        return static_cast<int>(code) < 10;
    }

    inline bool isConfigError(ErrorCode code)
    {
        return static_cast<int>(code) >= 10 && static_cast<int>(code) < 20;
    }

    inline bool isWifiError(ErrorCode code)
    {
        return static_cast<int>(code) >= 20 && static_cast<int>(code) < 30;
    }

    inline bool isMqttError(ErrorCode code)
    {
        return static_cast<int>(code) >= 30 && static_cast<int>(code) < 40;
    }

    inline bool isSensorError(ErrorCode code)
    {
        return static_cast<int>(code) >= 40 && static_cast<int>(code) < 50;
    }

    inline bool isRecoveryError(ErrorCode code)
    {
        return static_cast<int>(code) >= 50 && static_cast<int>(code) < 60;
    }

    inline bool isBluetoothError(ErrorCode code)
    {
        return static_cast<int>(code) >= 60 && static_cast<int>(code) < 70;
    }
};

#endif