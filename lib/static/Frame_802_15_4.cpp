#include "Frame_802_15_4.h"

static void uint64_to_bytes_le(uint8_t *dest, uint64_t value, int len)
{
    for (int i = 0; i < len; ++i)
    {
        dest[i] = (uint8_t)(value >> (i * 8));
    }
}
static uint64_t bytes_to_uint64_le(const uint8_t *src, int len)
{
    uint64_t value = 0;
    for (int i = 0; i < len; ++i)
    {
        value |= (uint64_t)src[i] << (i * 8);
    }
    return value;
}

// ---- Klassenimplementierung ----
Frame_802_15_4::Frame_802_15_4() : currentLength(0)
{
    memset(buffer, 0, MAX_UWB_MESSAGE_LEN);
}

// ---- Private Hilfsmethoden ----
uint8_t Frame_802_15_4::getDestAddrMode() const { return (buffer[1] >> 2) & 0x03; }
uint8_t Frame_802_15_4::getSourceAddrMode() const { return (buffer[1] >> 6) & 0x03; }
uint8_t Frame_802_15_4::getSequenceNumber() const { return buffer[2]; }

uint8_t Frame_802_15_4::getHeaderLength() const
{
    uint8_t destMode = getDestAddrMode();
    uint8_t srcMode = getSourceAddrMode();
    if (destMode < 2 || srcMode < 2)
        return 0;

    uint8_t destLen = (destMode == 2) ? 2 : 8; // 2=Short, 3=Extended
    uint8_t srcLen = (srcMode == 2) ? 2 : 8;

    // Header-Länge: FrameControl(2) + SeqNum(1) + PAN ID(2) + Adressen
    return 5 + destLen + srcLen;
}

// ---- Builder-Methoden ----
void Frame_802_15_4::buildRangingMessage(uint8_t seqNum, uint8_t funcCode, uint8_t destShortAddr, uint8_t sourceUid)
{
    buffer[0] = 0x41;
    buffer[1] = 0x88; // Dest: Short, Src: Short
    buffer[MSG_SN_IDX] = seqNum;
    buffer[3] = 0xCA;
    buffer[4] = 0xDE;
    buffer[5] = destShortAddr;
    buffer[6] = 0;
    buffer[MSG_SID_IDX] = sourceUid;
    buffer[8] = 0;
    buffer[MSG_FUNC_IDX] = funcCode;
    for (int i = 10; i < RANGING_MSG_LEN - FCS_LEN; ++i)
    {
        buffer[i] = 0;
    }
    currentLength = RANGING_MSG_LEN;
}

void Frame_802_15_4::buildDiscoveryBroadcast(uint8_t seqNum, uint64_t sourceMac)
{
    buffer[0] = 0x41;
    buffer[1] = 0xC8;
    buffer[2] = seqNum;
    buffer[3] = 0xCA;
    buffer[4] = 0xDE;
    buffer[5] = 0xFF;
    buffer[6] = 0xFF;                             // Broadcast Zieladresse
    uint64_to_bytes_le(&buffer[7], sourceMac, 8); // 8 Bytes für Extended Address
    uint8_t headerLength = 15;                    // 2(FC)+1(Seq)+2(PAN)+2(Dest)+8(Src)
    buffer[headerLength] = FUNC_CODE_DISCOVERY_BROADCAST;
    currentLength = headerLength + 1 + FCS_LEN;
}

void Frame_802_15_4::buildDiscoveryBlink(uint8_t seqNum, uint64_t destMac, uint64_t sourceMac)
{
    buffer[0] = 0x41;
    buffer[1] = 0xCC;
    buffer[2] = seqNum;
    buffer[3] = 0xCA;
    buffer[4] = 0xDE;
    uint64_to_bytes_le(&buffer[5], destMac, 8);
    uint64_to_bytes_le(&buffer[13], sourceMac, 8);
    uint8_t headerLength = 21; // 2(FC)+1(Seq)+2(PAN)+8(Dest)+8(Src)
    buffer[headerLength] = FUNC_CODE_DISCOVERY_BLINK;
    currentLength = headerLength + 1 + FCS_LEN;
}

void Frame_802_15_4::buildRangingConfig(uint8_t seqNum, uint64_t sourceMac, uint64_t destinationMac, uint8_t initiatorUid, uint8_t assignedResponderUid, uint8_t totalDevices)
{
    // --- Header aufbauen
    buffer[0] = 0x41;
    buffer[1] = 0xCC;
    buffer[2] = seqNum;
    buffer[3] = 0xCA;
    buffer[4] = 0xDE;
    uint64_to_bytes_le(&buffer[5], destinationMac, 8);
    uint64_to_bytes_le(&buffer[13], sourceMac, 8);
    uint8_t headerLength = 21;
    buffer[headerLength] = FUNC_CODE_RANGING_CONFIG;
    buffer[headerLength + 1] = initiatorUid;
    buffer[headerLength + 2] = assignedResponderUid;
    buffer[headerLength + 3] = totalDevices;
    uint16_t payloadLength = 4;
    currentLength = headerLength + payloadLength;
}

void Frame_802_15_4::parse(const uint8_t *rxBuffer, uint16_t length)
{
    if (length > MAX_UWB_MESSAGE_LEN)
        return;
    memcpy(buffer, rxBuffer, length);
    currentLength = length;
}
bool Frame_802_15_4::parseRangingConfig(uint8_t &initiatorUid, uint8_t &assignedResponderUid, uint8_t &totalDevices) const
{
    if (getFunctionCode() != FUNC_CODE_RANGING_CONFIG)
    {
        return false;
    }
    uint8_t headerLen = getHeaderLength();
    if (currentLength < headerLen + 4)
    {
        return false;
    }
    uint16_t payloadDataOffset = headerLen + 1;

    initiatorUid = buffer[payloadDataOffset];
    assignedResponderUid = buffer[payloadDataOffset + 1];
    totalDevices = buffer[payloadDataOffset + 2];
    return true;
}

// ---- Getter-Methoden ----
uint8_t Frame_802_15_4::getFunctionCode() const
{
    uint8_t headerLen = getHeaderLength();
    if (headerLen > 0 && currentLength > headerLen)
    {
        return buffer[headerLen];
    }
    return 0;
}

uint64_t Frame_802_15_4::getSourceMac() const
{
    if (getSourceAddrMode() == 3)
    { // 3 = Extended
        uint8_t destLen = (getDestAddrMode() == 2) ? 2 : 8;
        return bytes_to_uint64_le(&buffer[5 + destLen], 8);
    }
    return 0;
}

uint64_t Frame_802_15_4::getDestinationMac() const
{
    if (getDestAddrMode() == 3)
    { // 3 = Extended
        return bytes_to_uint64_le(&buffer[5], 8);
    }
    return 0;
}

uint8_t Frame_802_15_4::getSourceUid() const
{
    if (getSourceAddrMode() == 2)
    { // 2 = Short Address
        return buffer[MSG_SID_IDX];
    }
    return 0;
}

// ---- Zeitstempel & API-Zugriff ----
void Frame_802_15_4::getTimestamp(uint32_t *timestamp) const
{
    *timestamp = 0;
    for (int i = 0; i < RESP_MSG_TS_LEN; i++)
    {
        *timestamp += (uint32_t)buffer[MSG_T_REPLY_IDX + i] << (i * 8);
    }
}

void Frame_802_15_4::setTimestamp(uint64_t timestamp)
{
    for (int i = 0; i < RESP_MSG_TS_LEN; i++)
    {
        buffer[MSG_T_REPLY_IDX + i] = (uint8_t)(timestamp >> (i * 8));
    }
}

uint16_t Frame_802_15_4::getSourceAddressShort() const
{
    if (getSourceAddrMode() == 2)
    { // 2 = Short Address
        uint8_t destLen = (getDestAddrMode() == 2) ? 2 : 8;
        uint8_t srcAddrOffset = 5 + destLen;
        return (uint16_t)buffer[srcAddrOffset] | ((uint16_t)buffer[srcAddrOffset + 1] << 8);
    }
    return 0;
}

uint8_t *Frame_802_15_4::getBuffer() { return buffer; }
uint16_t Frame_802_15_4::getLength() const { return currentLength; }

// debug print
void Frame_802_15_4::print() const
{
    uint8_t funcCode = getFunctionCode();
    Serial.println("--- UWB Message ---");
    Serial.print("| Seq: ");
    Serial.println(getSequenceNumber());
    if (getSourceAddrMode() == 2 && getDestAddrMode() == 2)
    {
        Serial.println("| Type: Ranging Message");
        Serial.print("| Src UID: ");
        Serial.println(getSourceAddressShort());
        Serial.print("| Dest UID: ");
        Serial.println((uint16_t)buffer[5] | ((uint16_t)buffer[6] << 8));
    }
    else
    {
        Serial.println("| Type: Discovery Message");
        Serial.print("| Src MAC: ");
        uint64_t srcMac = getSourceMac();
        char macStr[13];
        sprintf(macStr, "%02x%02x%02x%02x%02x%02x",
                (uint8_t)(srcMac >> 40), (uint8_t)(srcMac >> 32),
                (uint8_t)(srcMac >> 24), (uint8_t)(srcMac >> 16),
                (uint8_t)(srcMac >> 8), (uint8_t)srcMac);
        Serial.println(macStr);

        Serial.print("| Dest MAC: ");
        uint64_t destMac = getDestinationMac();
        sprintf(macStr, "%02x%02x%02x%02x%02x%02x",
                (uint8_t)(destMac >> 40), (uint8_t)(destMac >> 32),
                (uint8_t)(destMac >> 24), (uint8_t)(destMac >> 16),
                (uint8_t)(destMac >> 8), (uint8_t)destMac);
        Serial.println(macStr);
    }

    Serial.print("| Func Code: 0x");
    Serial.println(funcCode, HEX);
    if (funcCode == FUNC_CODE_ACK || funcCode == FUNC_CODE_FINAL)
    {
        uint32_t ts = 0;
        getTimestamp(&ts);
        Serial.print("| Timestamp: ");
        Serial.println(ts);
    }
    Serial.println("---------------------");
}