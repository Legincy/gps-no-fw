#include "Frame_802_15_4.h"
#include "managers/UWBManager.h"
#include "WiFi.h"
#include <Arduino.h>
#include <algorithm>
#include <vector>

static void mac_uint64_to_str(uint64_t mac, char *str)
{
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
            (uint8_t)(mac >> 40), (uint8_t)(mac >> 32), (uint8_t)(mac >> 24),
            (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)(mac));
}

// DW3000-Konfiguration
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type */
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    DWT_PHRRATE_STD, /* PHY header rate. */
    (129 + 8 - 8),   /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
    DWT_STS_MODE_OFF,
    DWT_STS_LEN_64,
    DWT_PDOA_M0};

extern dwt_txconfig_t txconfig_options;

// --- Singleton Implementierung ---
UWBManager &UWBManager::getInstance()
{
    static UWBManager instance;
    return instance;
}

// --- Konstruktor Implementierung ---
UWBManager::UWBManager() : UID(0), INITIATOR_UID(0), NUM_NODES(0), WAIT_NUM(0),
                           deviceState(DISCOVERY), myMacAddress(0), frame_seq_nb(0),
                           status_reg(0), wait_poll(true), wait_ack(false), wait_range(false),
                           wait_final(false), counter(0), ret(0), discovered_count(0),
                           known_devices_count(0), poll_tx_ts(0), poll_rx_ts(0), range_tx_ts(0),
                           ack_tx_ts(0), range_rx_ts(0), t_reply_2(0), tof(0.0), distance(0.0),
                           previous_debug_millis(0), current_debug_millis(0), tx_time(0), tx_ts(0),
                           m_rangingCycleCompleted(false)
{
    // Arrays bei der Initialisierung leeren
    memset(target_uids, 0, sizeof(target_uids));
    memset(discovered_macs, 0, sizeof(discovered_macs));
    memset(known_devices, 0, sizeof(known_devices));
    memset(t_reply_1, 0, sizeof(t_reply_1));
    memset(t_round_1, 0, sizeof(t_round_1));
    memset(t_round_2, 0, sizeof(t_round_2));
}

// --- Öffentliche Methoden Implementierung ---

void UWBManager::start_uwb()
{
    WiFi.mode(WIFI_MODE_STA);
    uint8_t mac[6];
    WiFi.macAddress(mac);
    myMacAddress = ((uint64_t)mac[0] << 40) | ((uint64_t)mac[1] << 32) |
                   ((uint64_t)mac[2] << 24) | ((uint64_t)mac[3] << 16) |
                   ((uint64_t)mac[4] << 8) | (uint64_t)mac[5];

    spiBegin(UWB_IRQ, UWB_RST);
    spiSelect(UWB_SS);

    while (!dwt_checkidlerc())
    {
        Serial.println("IDLE FAILED");
        while (1)
            ;
    }
    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR)
    {
        Serial.println("INIT FAILED");
        while (1)
            ;
    }
    dwt_setleds(DWT_LEDS_DISABLE);
    if (dwt_configure(&config))
    {
        Serial.println("CONFIG FAILED");
        while (1)
            ;
    }

    dwt_configuretxrf(&txconfig_options);
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);
    dwt_setrxaftertxdelay(TX_TO_RX_DLY_UUS);
    dwt_setrxtimeout(RX_TIMEOUT_UUS); // TODO
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("My UWB MAC Address: ");
    Serial.println(macStr);
    Serial.println("UWB Setup complete.");
}

void UWBManager::setRangingConfiguration(uint8_t initiatorUid, uint8_t myAssignedUid, uint8_t totalDevices)
{
    UID = myAssignedUid;
    INITIATOR_UID = initiatorUid;
    NUM_NODES = totalDevices;
    WAIT_NUM = (UID > INITIATOR_UID) ? (UID - INITIATOR_UID) / 4 : 0;
    set_target_uids(); // Private Hilfsfunktion aufrufen
}

void UWBManager::initiator()
{
    if (deviceState == DISCOVERY)
    {
        if (INITIATOR_UID == 0)
        {
            INITIATOR_UID = (uint8_t)random(1, 50) * 4 + 2;
        }

        txMessage.buildDiscoveryBroadcast(frame_seq_nb++, myMacAddress);
        dwt_writetxdata(txMessage.getLength() - 2, txMessage.getBuffer(), 0);
        dwt_writetxfctrl(txMessage.getLength(), 0, 0);
        dwt_starttx(DWT_START_TX_IMMEDIATE);
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
            ;
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

        discovered_count = 0;
        memset(discovered_macs, 0, sizeof(discovered_macs));
        unsigned long discovery_start_time = millis();

        while (millis() - discovery_start_time < DISCOVERY_WINDOW_MS)
        {
            dwt_rxenable(DWT_START_RX_IMMEDIATE);
            while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
                ;

            if (status_reg & SYS_STATUS_RXFCG_BIT_MASK)
            {
                uint16_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
                dwt_readrxdata(rx_buffer, frame_len, 0);
                rxMessage.parse(rx_buffer, frame_len);

                if (rxMessage.getFunctionCode() == FUNC_CODE_DISCOVERY_BLINK && rxMessage.getDestinationMac() == myMacAddress)
                {
                    uint64_t responderMac = rxMessage.getSourceMac();
                    bool already_found = false;
                    for (int i = 0; i < discovered_count; i++)
                    {
                        if (discovered_macs[i] == responderMac)
                        {
                            already_found = true;
                            break;
                        }
                    }
                    if (!already_found && discovered_count < (MAX_NODES - 1))
                    {
                        discovered_macs[discovered_count++] = responderMac;
                    }
                }
            }
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO | SYS_STATUS_RXFCG_BIT_MASK);
        }

        if (discovered_count > 0)
        {
            known_devices_count = 0;
            uint8_t total_devices = discovered_count + 1;
            for (int i = 0; i < discovered_count; i++)
            {
                uint64_t responder_mac = discovered_macs[i];
                uint8_t assigned_uid = INITIATOR_UID + ((i + 1) * 4);
                updateKnownDevices(responder_mac, assigned_uid);
                txMessage.buildRangingConfig(frame_seq_nb++, myMacAddress, responder_mac, INITIATOR_UID, assigned_uid, total_devices);
                dwt_writetxdata(txMessage.getLength(), txMessage.getBuffer(), 0);
                dwt_writetxfctrl(txMessage.getLength() + 2, 0, 0); // FCS_LEN ist 2
                dwt_starttx(DWT_START_TX_IMMEDIATE);
                delay(15);
            }
            setRangingConfiguration(INITIATOR_UID, INITIATOR_UID, total_devices);
            deviceState = RANGING;
            wait_ack = false;
            wait_final = false;
            counter = 0;
        }
        else
        {
            delay(1000);
        }
    }
    else if (deviceState == RANGING)
    {
        if (!wait_ack && !wait_final && (counter == 0))
        {
            wait_ack = true;
            txMessage.buildRangingMessage(frame_seq_nb, FUNC_CODE_POLL, 0, UID);
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
            dwt_writetxdata(txMessage.getLength(), txMessage.getBuffer(), 0);
            dwt_writetxfctrl(txMessage.getLength(), 0, 1);
            dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
        }
        else
        {
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
            dwt_rxenable(DWT_START_RX_IMMEDIATE);
        }

        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
            ;

        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK)
        {
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
            uint16_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
            dwt_readrxdata(rx_buffer, frame_len, 0);
            rxMessage.parse(rx_buffer, frame_len);

            if (rxMessage.getSourceUid() != target_uids[counter])
            {
                // Falsche UID, zurücksetzen
                counter = 0;
                wait_ack = false;
                wait_final = false;
                return;
            }
            if (wait_ack)
            {
                poll_tx_ts = get_tx_timestamp_u64();
                t_round_1[counter] = get_rx_timestamp_u64() - poll_tx_ts;
                rxMessage.getTimestamp(&t_reply_1[counter]);
                counter++;
            }
            else
            {
                rxMessage.getTimestamp(&t_round_2[counter]);
                counter++;
            }
        }
        else
        { // Timeout oder Fehler
            txMessage.buildRangingMessage(frame_seq_nb, FUNC_CODE_RESET, 0, UID);
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
            dwt_writetxdata(txMessage.getLength(), txMessage.getBuffer(), 0);
            dwt_writetxfctrl(txMessage.getLength(), 0, 1);
            dwt_starttx(DWT_START_TX_IMMEDIATE);
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
            deviceState = DISCOVERY;
            wait_ack = false;
            wait_final = false;
            counter = 0;
            delay(1);
            return;
        }

        if (wait_ack && (counter == NUM_NODES - 1))
        {
            tx_time = (get_rx_timestamp_u64() + (RX_TO_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
            dwt_setdelayedtrxtime(tx_time);
            txMessage.buildRangingMessage(frame_seq_nb, FUNC_CODE_RANGE, 0, UID);
            dwt_writetxdata(txMessage.getLength(), txMessage.getBuffer(), 0);
            dwt_writetxfctrl(txMessage.getLength(), 0, 1);
            if (dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) == DWT_SUCCESS)
            {
                while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
                    ;
                wait_ack = false;
                wait_final = true;
                counter = 0;
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
            }
            return;
        }

        if (wait_final && (counter == NUM_NODES - 1))
        {
            current_debug_millis = millis();
            Serial.print(current_debug_millis - previous_debug_millis);
            Serial.print("ms\t");
            for (int i = 0; i < counter; i++)
            {
                t_reply_2 = get_tx_timestamp_u64() - (t_round_1[i] + poll_tx_ts);
                tof = ((double)t_round_1[i] * t_round_2[i] - (double)t_reply_1[i] * t_reply_2) /
                      ((double)t_round_1[i] + t_round_2[i] + t_reply_1[i] + t_reply_2);
                distance = tof * DWT_TIME_UNITS * SPEED_OF_LIGHT;
                updateDistance(target_uids[i], distance);
                Serial.print(target_uids[i]);
                Serial.print("\t");
                Serial.print(distance);
                Serial.print(" m\t");
            }
            Serial.println();
            previous_debug_millis = current_debug_millis;
            counter = 0;
            wait_ack = false;
            wait_final = false;
            frame_seq_nb++;
            m_rangingCycleCompleted = true;
            delay(INTERVAL);
        }
    }
}

void UWBManager::responder()
{
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR)))
        ;

    if (status_reg & SYS_STATUS_RXFCG_BIT_MASK)
    {
        uint16_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
        dwt_readrxdata(rx_buffer, frame_len, 0);
        rxMessage.parse(rx_buffer, frame_len);
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

        if (deviceState == DISCOVERY)
        {
            if (rxMessage.getFunctionCode() == FUNC_CODE_RANGING_CONFIG)
            {
                if (rxMessage.getDestinationMac() == myMacAddress)
                {
                    uint8_t initiator_uid, my_assigned_uid, total_devices;
                    if (rxMessage.parseRangingConfig(initiator_uid, my_assigned_uid, total_devices))
                    {
                        setRangingConfiguration(initiator_uid, my_assigned_uid, total_devices);
                        deviceState = RANGING;
                        wait_poll = true;
                        wait_range = false;
                        counter = 0;
                        return;
                    }
                }
            }
            if (rxMessage.getFunctionCode() == FUNC_CODE_DISCOVERY_BROADCAST)
            {
                delay(random(0, MAX_RESPONSE_DELAY_MS));
                uint64_t initiatorMac = rxMessage.getSourceMac();
                if (initiatorMac != 0)
                {
                    txMessage.buildDiscoveryBlink(rxMessage.getSequenceNumber(), initiatorMac, myMacAddress);
                    dwt_writetxdata(txMessage.getLength() - 2, txMessage.getBuffer(), 0); // FCS_LEN=2
                    dwt_writetxfctrl(txMessage.getLength(), 0, 0);
                    dwt_starttx(DWT_START_TX_IMMEDIATE);
                    while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
                        ;
                    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
                    return;
                }
            }
        }
        else if (deviceState == RANGING)
        {
            if (rxMessage.getFunctionCode() == FUNC_CODE_RESET || rxMessage.getFunctionCode() == FUNC_CODE_DISCOVERY_BROADCAST)
            {
                deviceState = DISCOVERY;
                wait_poll = true;
                wait_range = false;
                counter = 0;
                return;
            }
            if (rxMessage.getSourceUid() != target_uids[counter])
            {
                wait_poll = true;
                wait_range = false;
                counter = 0;
                return;
            }
            if (wait_poll)
            {
                if (counter == 0)
                {
                    poll_rx_ts = get_rx_timestamp_u64();
                }
                counter++;
            }
            else if (wait_range)
            {
                if (counter == 0)
                {
                    range_rx_ts = get_rx_timestamp_u64();
                }
                counter++;
            }
        }
    }
    else
    { // Fehler beim Empfang
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);
        wait_poll = true;
        wait_range = false;
        counter = 0;
        return;
    }

    if (deviceState == RANGING)
    {
        if (wait_poll && counter == WAIT_NUM)
        {
            tx_time = (get_rx_timestamp_u64() + (RX_TO_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
            dwt_setdelayedtrxtime(tx_time);
            tx_ts = (((uint64_t)(tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;
            txMessage.buildRangingMessage(frame_seq_nb, FUNC_CODE_ACK, 0, UID);
            txMessage.setTimestamp(tx_ts - poll_rx_ts);
            dwt_writetxdata(txMessage.getLength(), txMessage.getBuffer(), 0);
            dwt_writetxfctrl(txMessage.getLength(), 0, 1);
            if (dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) == DWT_SUCCESS)
            {
                while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
                    ;
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
            }
        }
        if (wait_poll && counter == NUM_NODES - 1)
        {
            counter = 0;
            wait_poll = false;
            wait_range = true;
            return;
        }
        if (wait_range && counter == WAIT_NUM)
        {
            ack_tx_ts = get_tx_timestamp_u64();
            tx_time = (get_rx_timestamp_u64() + (RX_TO_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
            dwt_setdelayedtrxtime(tx_time);
            txMessage.buildRangingMessage(frame_seq_nb, FUNC_CODE_FINAL, 0, UID);
            txMessage.setTimestamp(range_rx_ts - ack_tx_ts);
            dwt_writetxdata(txMessage.getLength(), txMessage.getBuffer(), 0);
            dwt_writetxfctrl(txMessage.getLength(), 0, 1);
            if (dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) == DWT_SUCCESS)
            {
                while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
                    ;
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
            }
        }
        if (wait_range && counter == NUM_NODES - 1)
        {
            counter = 0;
            wait_poll = true;
            wait_range = false;
            frame_seq_nb++;
            return;
        }
    }
}

// --- Private Hilfsfunktionen Implementierung ---

void UWBManager::updateKnownDevices(uint64_t mac, uint8_t uid)
{
    for (int i = 0; i < known_devices_count; i++)
    {
        if (known_devices[i].mac_address == mac)
        {
            known_devices[i].uid = uid;
            known_devices[i].distance = -1; // -1 als Indikator für "noch nicht gemessen"
            return;
        }
    }
    if (known_devices_count < MAX_NODES)
    {
        known_devices[known_devices_count].mac_address = mac;
        known_devices[known_devices_count].uid = uid;
        known_devices[known_devices_count].distance = 0.0;
        known_devices_count++;
    }
}

void UWBManager::updateDistance(uint8_t uid, double new_distance)
{
    for (int i = 0; i < known_devices_count; i++)
    {
        if (known_devices[i].uid == uid)
        {
            known_devices[i].distance = new_distance;
            return;
        }
    }
}

void UWBManager::set_target_uids()
{
    if (NUM_NODES <= 1)
        return;
    int target_idx = 0;
    if (UID != INITIATOR_UID)
    {
        target_uids[target_idx++] = INITIATOR_UID;
    }
    for (int i = 1; i < NUM_NODES; i++)
    {
        uint8_t responder_uid = INITIATOR_UID + (i * 4);
        if (responder_uid != UID)
        {
            if (target_idx < (NUM_NODES - 1))
            {
                target_uids[target_idx++] = responder_uid;
            }
        }
    }
}
bool UWBManager::performRangingCycleAndCreatePayload(JsonDocument *jsonData)
{
    this->m_rangingCycleCompleted = false;
    this->initiator();

    if (this->m_rangingCycleCompleted)
    {
        std::vector<RangingPartner> sorted_devices;
        for (int i = 0; i < this->known_devices_count; ++i)
        {
            sorted_devices.push_back(this->known_devices[i]);
        }
        std::sort(sorted_devices.begin(), sorted_devices.end(),
                  [](const RangingPartner &a, const RangingPartner &b)
                  {
                      return a.mac_address < b.mac_address;
                  });

        jsonData->clear();
        char sourceMacStr[18];
        mac_uint64_to_str(this->myMacAddress, sourceMacStr);
        (*jsonData)["source"] = sourceMacStr;
        JsonObject data = (*jsonData)["data"].to<JsonObject>();
        data["type"] = "uwb";
        JsonArray results = data["results"].to<JsonArray>();

        for (const auto &device : sorted_devices)
        {
            if (device.distance > 0)
            {
                JsonObject result = results.add<JsonObject>();
                char targetMacStr[18];
                mac_uint64_to_str(device.mac_address, targetMacStr);

                result["target_mac"] = targetMacStr;
                result["distance"] = serialized(String(device.distance, 2));
            }
        }
        return true;
    }

    return false;
}
void UWBManager::print_frame_data(const uint8_t *data, uint16_t length)
{
    Serial.print("DEBUG RX (len=");
    Serial.print(length);
    Serial.print("): ");
    for (int i = 0; i < length; ++i)
    {
        if (data[i] < 0x10)
            Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}
bool UWBManager::isRangingCycleComplete() const
{
    return m_rangingCycleCompleted;
}

void UWBManager::resetRangingCycleStatus()
{
    m_rangingCycleCompleted = false;
}

const UWBManager::RangingPartner *UWBManager::getKnownDevices() const
{
    return known_devices;
}

int UWBManager::getKnownDevicesCount() const
{
    return known_devices_count;
}

uint64_t UWBManager::getMyMacAddress() const
{
    return myMacAddress;
}