#ifndef UWB_MANAGER_H
#define UWB_MANAGER_H

#include "LogManager.h"
#include "dw3000.h"
#include "ArduinoJson.h"
#include "SPI.h"

#define INTERVAL 5
#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

#define UWB_RST 27
#define UWB_IRQ 34
#define UWB_SS 4
#define FUNC_CODE_POLL 0xE2
#define FUNC_CODE_ACK 0xE3
#define FUNC_CODE_RANGE 0xE4
#define FUNC_CODE_FINAL 0xE5
#define FUNC_CODE_RESET 0xE6

#define MSG_LEN 16      /* message length */
#define BUF_LEN MSG_LEN /* buffer length */
#define MSG_SN_IDX 2    /* sequence number */
#define MSG_SID_IDX 7   /* source id */
#define MSG_FUNC_IDX 9  /* func code*/

#define MSG_T_REPLY_IDX 10 /* byte index of transmitter ts */
#define RESP_MSG_TS_LEN 4
#define TX_TO_RX_DLY_UUS 100
#define RX_TO_TX_DLY_UUS 800
#define RX_TIMEOUT_UUS 400000

#define MAX_DEVICES 10 // Maximale Anzahl von Geräten im Cluster

enum UWBState
{
    NOT_INIT = 0,
    IDLE = 1,
    CLUSTER_UPDATE = 2,
    READY = 3
};

enum DeviceType
{
    TAG = 0,
    ANCHOR = 1,
    NONE = -1
};

struct Node
{
    char address[13];
    uint8_t UID = 99;
    uint8_t WAIT_NUM = 99;
    DeviceType type = NONE;
    float raw_distance = 0;
};

struct Cluster
{
    char name[20];
    Node *devices[MAX_DEVICES];
    uint8_t deviceCount = 0;
};

extern uint8_t tx_msg[], rx_msg[];
extern uint8_t frame_seq_nb;
extern uint8_t rx_buffer[BUF_LEN];
// extern int target_uids[NUM_DW3000 - 1];
extern uint32_t status_reg;
extern bool wait_poll, wait_ack, wait_range, wait_final;
extern int counter;
extern int ret;

extern uint64_t poll_tx_ts, poll_rx_ts, range_tx_ts, ack_tx_ts, range_rx_ts;
extern uint32_t t_reply_1[MAX_DEVICES - 1];
extern uint64_t t_reply_2;
extern uint64_t t_round_1[MAX_DEVICES - 1];
extern uint32_t t_round_2[MAX_DEVICES - 1];
// extern uint8_t target_uids[];
extern double tof, distance;
extern unsigned long previous_debug_millis, current_debug_millis;
extern int millis_since_last_serial_print;
extern uint32_t tx_time;
extern uint64_t tx_ts;
extern float clockOffsetRatioAck, clockOffsetRatioFinal;
class UWBManager
{
private:
    // Privater Konstruktor (Singleton)
    UWBManager() : log(LogManager::getInstance()), configManager(ConfigManager::getInstance())
    {
        RuntimeConfig &config = configManager.getRuntimeConfig();
        Node node;
        thisDevice = node;
        strncpy(thisDevice.address, config.device.modifiedMac, sizeof(thisDevice.address));
    }

    UWBState currentState = NOT_INIT;
    LogManager &log;
    ConfigManager &configManager;
    float distances[MAX_DEVICES - 1];
    Node thisDevice;
    Cluster currentCluster;
    uint8_t target_uids[MAX_DEVICES - 1];
    uint8_t tx_msg[MSG_LEN] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t rx_msg[MSG_LEN] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0};

public:
    /**
     * @brief Gibt die Singleton-Instanz von UWBManager zurück.
     *
     * @return Referenz auf die einzige Instanz von UWBManager.
     */
    static UWBManager &getInstance()
    {
        static UWBManager instance;
        return instance;
    }

    /**
     * @brief Startet den UWB-Betrieb.
     *
     * Führt alle notwendigen Initialisierungen für UWB durch.
     */
    bool begin();

    /**
     * @brief Führt die Initiator-Routine aus.
     *
     * Diese Methode kann als Startpunkt für eine UWB-Initiator-Routine verwendet werden.
     */
    void initiator();

    /**
     * @brief Führt die Responder-Routine aus.
     *
     * Diese Methode kann als Startpunkt für eine UWB-Responder-Routine verwendet werden.
     */
    void responder();

    // Methode, um den Gerätetyp von außen zu setzen
    void setDeviceType(const char *typeStr);

    bool updateClusterFromMqtt(const char *payload);

    float *getDistances();

    bool updateClusterFromServer();

    void debugPrint();
    void loop();
    void set_target_uids();
    uint32_t waitForReceptionEvent(uint32_t eventMask);
    bool sendTxMessage(uint8_t *msg, uint16_t len, uint32_t txFlags);

    // // Initiator-spezifische Funktionen
    void initiatorSendPoll();
    void initiatorSendRange();

    // // Responder-spezifische Funktionen
    void responderSendAck();
    void responderSendFinal();
    void processResponderMessage();
    void changeState(UWBState state);

    bool getDistanceJson(JsonDocument &doc, const Node *targetDevice);
    void clearCluster();
    Cluster &getCluster() { return currentCluster; };
};

// Gemeinsame Hilfsfunktionen

#endif
