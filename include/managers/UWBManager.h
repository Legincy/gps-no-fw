#pragma once

#ifndef UWBManager_H
#define UWBManager_H

#include "dw3000.h"
#include "Frame_802_15_4.h"
#include "ArduinoJson.h"
#include "ConfigManager.h"
#include "LogManager.h"
#include "MQTTManager.h"
// Function code
static const uint8_t FUNC_CODE_POLL = 0xE2, FUNC_CODE_ACK = 0xE3, FUNC_CODE_RANGE = 0xE4,
                     FUNC_CODE_FINAL = 0xE5, FUNC_CODE_RESET = 0xE6,
                     FUNC_CODE_DISCOVERY_BROADCAST = 0xD1, FUNC_CODE_DISCOVERY_BLINK = 0xD2,
                     FUNC_CODE_RANGING_CONFIG = 0x20;

// --- MAX NODES ---
static const int MAX_NODES = 7;

class UWBManager
{
public:
    static UWBManager &getInstance();
    // --- Öffentliche Strukturen & Enums ---
    enum DeviceState
    {
        DISCOVERY,
        RANGING
    };
    struct RangingPartner
    {
        uint64_t mac_address;
        uint8_t uid;
        double distance;
    };

    UWBManager(UWBManager const &) = delete;
    void operator=(UWBManager const &) = delete;

    // --- Öffentliche Methoden ---
    void start_uwb();
    void initiator();
    void responder();
    void responder_loop();
    bool initiator_loop();
    const RangingPartner *getKnownDevices() const;
    void loop();
    bool enableInitator();
    bool disableInitator();

private:
    UWBManager(); // Privater Konstruktor
    // Hilf-Managers
    RuntimeConfig &runtimeconfig;
    LogManager &logManager;
    MQTTManager &mqttManager;
    unsigned long last_distance_publish = 0;
    // JSON Dokument für MQTT Nachrichten
    JsonDocument jsonDoc;
    // --- Private Konstanten ---
    static const int INTERVAL = 5;
    static const int UWB_RST = 27, UWB_IRQ = 34, UWB_SS = 4;

    static const uint16_t TX_ANT_DLY = 16385, RX_ANT_DLY = 16385;
    static const int TX_TO_RX_DLY_UUS = 100, RX_TO_TX_DLY_UUS = 800, RX_TIMEOUT_UUS = 400000;
    static const int BUF_LEN = 127;
    static const int DISCOVERY_WINDOW_MS = 1000;
    static const int MAX_RESPONSE_DELAY_MS = 50;

    // --- Private Member-Variablen ---
    // Ranging & Device Config
    uint8_t UID, INITIATOR_UID;
    int NUM_NODES, WAIT_NUM;
    DeviceState deviceState;
    uint64_t myMacAddress;
    bool m_rangingCycleCompleted;
    bool configInitatorMode = false;
    bool configResponderMode = false;
    // Messages & Buffers
    Frame_802_15_4 txMessage, rxMessage;
    uint8_t rx_buffer[BUF_LEN];
    uint8_t frame_seq_nb;

    // Status & Logic Flow
    uint32_t status_reg;
    int target_uids[MAX_NODES - 1];
    bool wait_poll, wait_ack, wait_range, wait_final;
    int counter, ret;
    // Discovery
    uint64_t discovered_macs[MAX_NODES - 1];
    int discovered_count;

    // Known Devices
    RangingPartner known_devices[MAX_NODES];
    int known_devices_count;

    // Timestamps
    uint64_t poll_tx_ts, poll_rx_ts, range_tx_ts, ack_tx_ts, range_rx_ts;
    uint32_t t_reply_1[MAX_NODES - 1];
    uint64_t t_reply_2;
    uint64_t t_round_1[MAX_NODES - 1];
    uint32_t t_round_2[MAX_NODES - 1];
    uint32_t tx_time;
    uint64_t tx_ts;

    // Calculated values
    double tof, distance;

    // Debug
    unsigned long previous_debug_millis, current_debug_millis;

    // --- Private Hilfsfunktionen ---
    int getKnownDevicesCount() const;
    uint64_t getMyMacAddress() const;
    void setRangingConfiguration(uint8_t initiatorUid, uint8_t myAssignedUid, uint8_t totalDevices);
    bool isRangingCycleComplete() const;
    void resetRangingCycleStatus();
    void updateKnownDevices(uint64_t mac, uint8_t uid);
    void updateDistance(uint8_t uid, double new_distance);
    void set_target_uids();
    void print_frame_data(const uint8_t *data, uint16_t length);
    void configInitator();
    void configResponder();
};

#endif