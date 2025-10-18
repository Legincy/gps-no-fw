#pragma once

#include <cstdint>
#include <cstring>
#include "dw3000_regs.h"

// ----------- Nachrichten-Layout-Definitionen (nur für Ranging) -----------
#define MSG_SN_IDX 2
#define MSG_DID_IDX 5
#define MSG_SID_IDX 7
#define MSG_FUNC_IDX 9
#define MSG_T_REPLY_IDX 10
#define RESP_MSG_TS_LEN 4
#define MAX_UWB_MESSAGE_LEN 127

void mac_uint64_to_str(uint64_t mac, char *str);

class Frame_802_15_4
{
public:
    static const uint16_t RANGING_MSG_LEN = 16;
    Frame_802_15_4();

    // ---- Builder-Methoden ----
    void buildRangingMessage(uint8_t seqNum, uint8_t funcCode, uint8_t destShortAddr, uint8_t sourceUid);
    void buildDiscoveryBroadcast(uint8_t seqNum, uint64_t sourceMac);
    void buildDiscoveryBlink(uint8_t seqNum, uint64_t destMac, uint64_t sourceMac);
    void buildRangingConfig(uint8_t seqNum, uint64_t sourceMac, uint64_t destinationMac, uint8_t initiatorUid, uint8_t assignedResponderUid, uint8_t totalDevices);

    // ---- Parser ----
    void parse(const uint8_t *rxBuffer, uint16_t length);
    bool parseRangingConfig(uint8_t &initiatorUid, uint8_t &assignedResponderUid, uint8_t &totalDevices) const;
    // ---- Getter ----
    uint8_t getFunctionCode() const;
    uint8_t getSourceUid() const;
    uint64_t getSourceMac() const;
    uint64_t getDestinationMac() const;
    uint16_t getSourceAddressShort() const;
    uint8_t getSequenceNumber() const;

    // ---- Zeitstempel & API-Zugriff ----
    void getTimestamp(uint32_t *timestamp) const;
    void setTimestamp(uint64_t timestamp);
    uint8_t *getBuffer();
    uint16_t getLength() const;
    void print() const;

private:
    uint8_t buffer[MAX_UWB_MESSAGE_LEN];
    uint16_t currentLength;
    uint8_t getSourceAddrMode() const;
    uint8_t getDestAddrMode() const;
    uint8_t getHeaderLength() const;
};