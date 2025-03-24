#ifndef ULTRAWIDEBAND_MANAGER_H
#define ULTRAWIDEBAND_MANAGER_H

#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

#define NUM_NODES 3
#define INTERVAL 5
#define UWB_RST 27
#define UWB_IRQ 34
#define UWB_SS 4

#define FUNC_CODE_POLL 0xE2
#define FUNC_CODE_ACK 0xE3
#define FUNC_CODE_RANGE 0xE4
#define FUNC_CODE_FINAL 0xE5
#define FUNC_CODE_RESET 0xE6

#define MSG_LEN 16         /* message length */
#define BUF_LEN MSG_LEN    /* buffer length */
#define MSG_SN_IDX 2       /* sequence number */
#define MSG_SID_IDX 7      /* source id */
#define MSG_FUNC_IDX 9     /* func code*/
#define MSG_T_REPLY_IDX 10 /* byte index of transmitter ts */
#define RESP_MSG_TS_LEN 4
#define TX_TO_RX_DLY_UUS 100
#define RX_TO_TX_DLY_UUS 800
#define RX_TIMEOUT_UUS 400000

#define NODE_NUM 0
#define WAIT_NUM 0

#define U0 0
#define U1 14
#define U2 18
#define U3 22
#define U4 26
#define U5 30
#define U6 34

#include <vector>
#include <dw3000.h>
#include "managers/ConfigManager.h"
#include "managers/LogManager.h"

enum class NodeType
{
    TAG,
    ANCHOR,
    UNKNOWN
};

enum class UltraWidebandState
{
    INACTIVE,
    INITIALIZING,
    ACTIVE,
    RANGING,
    ERROR
};

enum class NodeId
{
    TAG,
    ANCHOR_U2,
    ANCHOR_U3,
    ANCHOR_U4,
    ANCHOR_U5,
    ANCHOR_U6,
};

struct RangingData
{
    uint16_t targetId;
    float distance;
    uint64_t timestamp;
};

struct DW3000
{
    dwt_config_t dwtConfig;
    dwt_txconfig_t txconfig;

    uint8_t irqPin;
    uint8_t rstPin;
    uint8_t ssPin;
    uint16_t txAntennaDelay;
    uint16_t rxAntennaDelay;
    uint16_t txAfterRxDelay;
    uint16_t rxTimeout;
};

struct MessageProtocol
{
    uint8_t tx_msg[16];
    uint8_t rx_msg[16];
    uint8_t frame_seq_nb;
    uint8_t rx_buffer[16];
    uint32_t status_reg;
};

struct Ranging
{
    bool wait_poll;
    bool wait_final;
    bool wait_ack;
    bool wait_range;
    int counter;
    int ret;
    uint64_t poll_tx_ts, poll_rx_ts, range_tx_ts, ack_tx_ts, range_rx_ts;
    uint32_t t_reply_1[NUM_NODES - 1];
    uint64_t t_reply_2;
    uint64_t t_round_1[NUM_NODES - 1];
    uint32_t t_round_2[NUM_NODES - 1];
    uint32_t tx_time;
    uint64_t tx_ts;
};

class UltraWidebandManager
{
private:
    UltraWidebandManager();

    LogManager &log;
    ConfigManager &configManager;

    UltraWidebandState state;
    NodeType nodeType;

    uint8_t nodeUID;
    uint8_t numNodes;
    uint8_t waitNum;
    uint32_t rangingInterval;

    uint32_t status_reg = 0;
    DW3000 dw3000;
    MessageProtocol messageProtocol;
    Ranging ranging;
    double tof, distance;

    std::vector<RangingData> measurements;
    std::vector<uint8_t> targetIds;
    uint32_t lastRangingTimestamp;
    bool rangingInProgress;
    bool autoRangingEnabled;

    bool initializeHardware();
    void configureDW3000();
    void configureTargetIds();

    void runInitiator();
    void runResponder();

    void updateMeasurement(uint8_t targetId, float distance);
    void resp_msg_get_ts(uint8_t *ts_field, uint32_t *ts);
    void resp_msg_set_ts(uint8_t *ts_field, uint64_t ts);

    static uint64_t get_tx_timestamp_u64();
    static uint64_t get_rx_timestamp_u64();
    static void Sleep(unsigned int time_ms);

    const char *getStateString(UltraWidebandState state) const;
    const char *getNodeTypeString(NodeType type) const;

public:
    UltraWidebandManager(const UltraWidebandManager &) = delete;
    void operator=(const UltraWidebandManager &) = delete;

    static UltraWidebandManager &getInstance()
    {
        static UltraWidebandManager instance;
        return instance;
    }

    bool begin();
    void update();
    void shutdown();

    void printStatus();

    void setNodeType(NodeType type);

    UltraWidebandState getState() const { return state; }
    const char *getStateString() const { return getStateString(state); }
    NodeType getNodeType() const { return nodeType; }
    const char *getNodeTypeString() const { return getNodeTypeString(nodeType); }
};

#endif
