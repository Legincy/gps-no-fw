#include "managers/UWBManager.h"

dwt_config_t config = {
    5,                /* Channel number. */
    DWT_PLEN_128,     /* Preamble length. Used in TX only. */
    DWT_PAC8,         /* Preamble acquisition chunk size. Used in RX only. */
    9,                /* TX preamble code. Used in TX only. */
    9,                /* RX preamble code. Used in RX only. */
    1,                /* 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2
                         for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type */
    DWT_BR_6M8,       /* Data rate. */
    DWT_PHRMODE_STD,  /* PHY header mode. */
    DWT_PHRRATE_STD,  /* PHY header rate. */
    (129 + 8 - 8),    /* SFD timeout (preamble length + 1 + SFD length - PAC
                         size).    Used in RX only. */
    DWT_STS_MODE_OFF, /* STS disabled */
    DWT_STS_LEN_64,   /* STS length see allowed values in Enum
                       * dwt_sts_lengths_e
                       */
    DWT_PDOA_M0       /* PDOA mode off */
};
extern dwt_txconfig_t txconfig_options;

uint8_t frame_seq_nb = 0;
uint8_t rx_buffer[BUF_LEN];

uint32_t status_reg = 0;
bool wait_poll = true;
bool wait_final = false;
bool wait_ack = true;
bool wait_range = false;
int counter = 0;
int ret;

uint64_t poll_tx_ts, poll_rx_ts, range_tx_ts, ack_tx_ts, range_rx_ts;
uint32_t t_reply_1[MAX_DEVICES - 1];
uint64_t t_reply_2;
uint64_t t_round_1[MAX_DEVICES - 1];
uint32_t t_round_2[MAX_DEVICES - 1];
double tof, distance;
unsigned long previous_debug_millis = 0;
unsigned long current_debug_millis = 0;
int millis_since_last_serial_print;
uint32_t tx_time;
uint64_t tx_ts;

void UWBManager::set_target_uids()
{
    int target_count = 0;
    for (int i = 0; i < currentCluster.deviceCount; i++)
    {
        Node *device = currentCluster.devices[i];
        if (device == nullptr)
            continue;
        if (device == &thisDevice)
            continue;
        target_uids[target_count++] = device->UID;
    }
}

bool UWBManager::begin()
{
    spiBegin(UWB_IRQ, UWB_RST);
    spiSelect(UWB_SS);

    if (!dwt_checkidlerc())
    {
        log.error("UWBManager", "IDLE FAILED");
        return false;
    }

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR)
    {
        log.error("UWBManager", "IDLE FAILED");
        return false;
    }

    dwt_setleds(DWT_LEDS_DISABLE);

    if (dwt_configure(&config))
    {
        log.error("UWBManager", "CONFIG FAILED");
        return false;
    }
    char msg[128];
    snprintf(msg, sizeof(msg), "Setup complete. Device: %s, UID: %d", DEVICE_NAME, thisDevice.UID);
    log.info("UWBManager", msg);
    changeState(IDLE);
    return true;
}

void UWBManager::initiator()
{
    if (!wait_ack && !wait_final && (counter == 0))
    {
        // Starte einen neuen Ranging-Zyklus: Poll-Nachricht senden
        initiatorSendPoll();
    }
    else
    {
        // Falls bereits ein Vorgang läuft: RX-Modus aktivieren
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
    }

    // Auf Empfang einer Nachricht warten (ACK oder Final)
    uint32_t status = waitForReceptionEvent(SYS_STATUS_RXFCG_BIT_MASK |
                                            SYS_STATUS_ALL_RX_TO |
                                            SYS_STATUS_ALL_RX_ERR);

    if (status & SYS_STATUS_RXFCG_BIT_MASK)
    {
        // Nachricht wurde erfolgreich empfangen
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
        dwt_readrxdata(rx_buffer, BUF_LEN, 0);
        if (rx_buffer[MSG_SID_IDX] != target_uids[counter])
        {
            // Falscher Absender, Vorgang zurücksetzen
            dwt_write32bitreg(SYS_STATUS_ID,
                              SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
            counter = 0;
            wait_ack = false;
            wait_final = false;
            return;
        }
        // Verarbeite empfangene ACK bzw. Final-Nachricht
        if (wait_ack)
        {
            poll_tx_ts = get_tx_timestamp_u64();
            t_round_1[counter] = get_rx_timestamp_u64() - poll_tx_ts;
            resp_msg_get_ts(&rx_buffer[MSG_T_REPLY_IDX], &t_reply_1[counter]);
            ++counter;
        }
        else
        {
            resp_msg_get_ts(&rx_buffer[MSG_T_REPLY_IDX], &t_round_2[counter]);
            ++counter;
        }
    }
    else
    {
        // Fehler oder Timeout: Sende Reset-Nachricht und setze Status zurück
        tx_msg[MSG_SN_IDX] = frame_seq_nb;
        tx_msg[MSG_FUNC_IDX] = FUNC_CODE_RESET;
        sendTxMessage(tx_msg, MSG_LEN, DWT_START_TX_IMMEDIATE);
        dwt_write32bitreg(SYS_STATUS_ID,
                          SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        wait_ack = false;
        wait_final = false;
        counter = 0;
        Sleep(1);
        return;
    }
    // Übergang vom Poll- zum Range-Modus
    if (wait_ack && (counter == currentCluster.deviceCount - 1))
    {
        initiatorSendRange();
        return;
    }
    // Verarbeitung der Final-Nachrichten: Distanzberechnung durchführen
    if (wait_final && (counter == currentCluster.deviceCount - 1))
    {
        range_tx_ts = get_tx_timestamp_u64();
        current_debug_millis = millis();
        // Serial.print(current_debug_millis - previous_debug_millis);
        // Serial.print("ms\t");
        for (int i = 0; i < counter; i++)
        {
            t_reply_2 = range_tx_ts - (t_round_1[i] + poll_tx_ts);
            tof = (t_round_1[i] * t_round_2[i] - t_reply_1[i] * t_reply_2) /
                  (t_round_1[i] + t_round_2[i] + t_reply_1[i] + t_reply_2) *
                  DWT_TIME_UNITS;
            distance = tof * SPEED_OF_LIGHT;
            if (distance > 1000)
            {
                distance = 0;
            }
            currentCluster.devices[i + 1]->raw_distance = distance;

            // snprintf(dist_str, sizeof(dist_str), "%3.3f m\t", currentCluster.devices[i]->raw_distance);
            // Serial.print(currentCluster.devices[i]->UID);
            // Serial.print("\t");
            // Serial.print(dist_str);
        }
        // Serial.println();

        previous_debug_millis = current_debug_millis;
        counter = 0;
        wait_ack = false;
        wait_final = false;
        ++frame_seq_nb;
        Sleep(INTERVAL);
    }
}

void UWBManager::responder()
{
    // RX-Modus sofort aktivieren
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    uint32_t status = waitForReceptionEvent(SYS_STATUS_RXFCG_BIT_MASK |
                                            SYS_STATUS_ALL_RX_ERR);

    if (status & SYS_STATUS_RXFCG_BIT_MASK)
    {
        processResponderMessage();
    }
    else
    {
        // Bei Fehler oder Timeout: Status zurücksetzen und Poll-Modus aktivieren
        wait_poll = true;
        wait_range = false;
        counter = 0;
        dwt_write32bitreg(SYS_STATUS_ID,
                          SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);
        return;
    }

    if (wait_poll)
    {
        if (counter == thisDevice.WAIT_NUM)
        {
            responderSendAck();
        }
        if (counter == currentCluster.deviceCount - 1)
        {
            // Wechsel in den Range-Modus, wenn alle ACKs empfangen wurden
            counter = 0;
            wait_poll = false;
            wait_range = true;
            return;
        }
    }
    if (wait_range)
    {
        if (counter == thisDevice.WAIT_NUM)
        {
            responderSendFinal();
        }
        if (counter == currentCluster.deviceCount - 1)
        {
            // Alle Final-Nachrichten empfangen: Zurücksetzen für den nächsten Zyklus
            counter = 0;
            wait_poll = true;
            wait_range = false;
            ++frame_seq_nb;
            return;
        }
    }
}

bool UWBManager::updateClusterFromServer()
{
    // JSON-Dokument parsen
    const char *payload = R"json(
        {
          "uwb": {
            "type": "TAG",
            "address": "C4153E8D3A08",
            "cluster": {
              "name": "Labor",
              "devices": [
                {"address": "F4183E8D3A08", "type": "ANCHOR"},
                {"address": "FCB30329E748", "type": "ANCHOR"}
              ]
            }
          }
        }
        )json";

    Node *anchors[MAX_DEVICES - 1];
    Node *tag;
    uint8_t anchorsCount = 0;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        log.error("UWBManager", "JSON Deserialization failed");
        return false;
    }

    JsonObject uwb = doc["uwb"];
    if (uwb.isNull())
    {
        log.error("UWBManager", "JSON: 'uwb'-Objekt nicht vorhanden");
        return false;
    }

    // Überprüfe die Adresse aus dem JSON
    const char *addrStr = uwb["address"];
    if (addrStr)
    {
        uint64_t jsonAddr = strtoull(addrStr, NULL, 16);
        if (jsonAddr != thisDevice.address)
        {
            log.error("UWBManager", "JSON address does not match device address");
            char addrBuf[17];
            snprintf(addrBuf, sizeof(addrBuf), "%012llX", thisDevice.address);
            Serial.print("thisDevice.address: ");
            Serial.println(addrBuf);
            return false;
        }
    }

    // Lese den Typ aus (TAG oder ANCHOR)
    const char *typeStr = uwb["type"];
    if (typeStr)
    {
        if (strcmp(typeStr, "TAG") == 0)
        {
            thisDevice.type = TAG;
            tag = &thisDevice;
        }
        else if (strcmp(typeStr, "ANCHOR") == 0)
        {
            thisDevice.type = ANCHOR;
            anchors[anchorsCount++] = &thisDevice;
        }
    }

    // Verarbeite den Cluster-Block aus dem JSON
    JsonObject cluster = uwb["cluster"];
    if (!cluster.isNull())
    {
        const char *clusterName = cluster["name"];
        if (clusterName)
        {
            strncpy(currentCluster.name, clusterName, sizeof(currentCluster.name) - 1);
            currentCluster.name[sizeof(currentCluster.name) - 1] = '\0';
        }

        // Lese das Devices-Array aus dem Cluster-Block
        JsonArray devices = cluster["devices"];
        if (!devices.isNull())
        {
            // Beginne bei Index 1, da Index 0 bereits thisDevice enthält
            for (JsonObject deviceObj : devices)
            {
                if (currentCluster.deviceCount >= MAX_DEVICES)
                    break;

                // Verwende currentCluster.devices[currentCluster.deviceCount] als nächsten freien Slot
                Node *pDevice = new Node;
                // Lese und konvertiere die Adresse
                const char *devAddrStr = deviceObj["address"];
                if (devAddrStr)
                {
                    pDevice->address = strtoull(devAddrStr, NULL, 16);
                }
                // Lese und setze den Typ
                const char *devTypeStr = deviceObj["type"];
                if (devTypeStr)
                {
                    if (strcmp(devTypeStr, "TAG") == 0)
                    {
                        pDevice->type = TAG;
                        tag = pDevice;
                    }
                    else if (strcmp(devTypeStr, "ANCHOR") == 0)
                    {
                        pDevice->type = ANCHOR;
                        anchors[anchorsCount++] = pDevice;
                    }
                }
            }
        }
    }

    // Sortieren Anchors
    for (int i = 0; i < anchorsCount - 1; i++)
    {
        for (int j = i + 1; j < anchorsCount; j++)
        {
            if (anchors[i]->address > anchors[j]->address)
            {
                Node *temp = anchors[i];
                anchors[i] = anchors[j];
                anchors[j] = temp;
            }
        }
    }
    // Füge Tag und Anchors zu currentCluster *devices array vergeben UID
    if (tag != nullptr)
    {
        currentCluster.devices[currentCluster.deviceCount++] = tag;
    }
    // Füge dann alle sortierten Anchors hinzu.
    for (int i = 0; i < anchorsCount; i++)
    {
        currentCluster.devices[currentCluster.deviceCount++] = anchors[i];
    }
    // UID-Zuweisung:
    // Das Tag (Index 0) erhält immer UID 14.
    if (currentCluster.deviceCount > 0)
    {
        currentCluster.devices[0]->UID = 14;
        currentCluster.devices[0]->WAIT_NUM = 0;
    }
    // Ab Index 1 erhalten die Geräte (Anchors) in Schritten von 4:
    for (int i = 1; i < currentCluster.deviceCount; i++)
    {
        currentCluster.devices[i]->UID = 14 + (i * 4);
        currentCluster.devices[i]->WAIT_NUM = i;
    }
    char msg[128];
    snprintf(msg, sizeof(msg), "Updated Nodes from %s", currentCluster.name);
    log.info("UWBManager", msg);
    // Debug-Ausgabe
    // Serial.println("----- UWBManager DebugPrint -----");
    // Serial.print("thisDevice.type: ");
    // Serial.println((thisDevice.type == TAG) ? "TAG" : "ANCHOR");

    // char addrBuf[17];
    // snprintf(addrBuf, sizeof(addrBuf), "%012llX", thisDevice.address);
    // Serial.print("thisDevice.address: ");
    // Serial.println(addrBuf);

    // Serial.print("Cluster name: ");
    // Serial.println(currentCluster.name);

    // Serial.print("Anzahl Devices: ");
    // Serial.println(currentCluster.deviceCount);

    // for (int i = 0; i < currentCluster.deviceCount; i++)
    // {
    //     char devAddrBuf[17];
    //     snprintf(devAddrBuf, sizeof(devAddrBuf), "%012llX", currentCluster.devices[i]->address);
    //     Serial.print(" Device ");
    //     Serial.print(i);
    //     Serial.print(" - Type: ");
    //     Serial.print((currentCluster.devices[i]->type == TAG) ? "TAG" : "ANCHOR");
    //     Serial.print(", Address: ");
    //     Serial.print(devAddrBuf);
    //     Serial.print(", UID: ");
    //     Serial.print(currentCluster.devices[i]->UID);
    //     Serial.print(", WAIT_NUM: ");
    //     Serial.println(currentCluster.devices[i]->WAIT_NUM);
    // }
    // Serial.println("---------------------------------");
    return true;
}

void UWBManager::debugPrint()
{
    Serial.println("----- UWBManager DebugPrint -----");

    // Ausgabe des Typs des eigenen Gerätes
    Serial.print("thisDevice.type: ");
    Serial.println((thisDevice.type == TAG) ? "TAG" : "ANCHOR");

    // Ausgabe des tx_msg-Inhalts
    Serial.print("tx_msg: ");
    for (uint8_t i = 0; i < MSG_LEN; i++)
    {
        // Formatierte Ausgabe jedes Bytes als 2-stellige Hexadezimalzahl
        char byteStr[3];
        snprintf(byteStr, sizeof(byteStr), "%02X", tx_msg[i]);
        Serial.print(byteStr);
        Serial.print(" ");
    }
    Serial.println();

    // Ausgabe der Adresse im Hex-Format (12-stellig)
    char addrBuf[17]; // 16 Zeichen max + '\0'
    snprintf(addrBuf, sizeof(addrBuf), "%012llX", thisDevice.address);
    Serial.print("thisDevice.address: ");
    Serial.println(addrBuf);

    // Ausgabe des Cluster-Namens
    Serial.print("Cluster name: ");
    Serial.println(currentCluster.name);

    // Ausgabe der Geräte im Cluster
    Serial.print("Anzahl Devices: ");
    Serial.println(currentCluster.deviceCount);
    for (uint8_t i = 0; i < currentCluster.deviceCount; i++)
    {
        Node *dev = currentCluster.devices[i];
        snprintf(addrBuf, sizeof(addrBuf), "%012llX", dev->address);
        Serial.print(" Device ");
        Serial.print(i);
        Serial.print(" - Type: ");
        Serial.print((dev->type == TAG) ? "TAG" : "ANCHOR");
        Serial.print(", Address: ");
        Serial.print(addrBuf);

        // Falls ein Ranging-Wert vorhanden ist, wird dieser ausgegeben.
        Serial.print(", raw_distance: ");
        Serial.println(dev->raw_distance);
    }
    for (int i = 0; i < currentCluster.deviceCount - 1; i++)
    {
        Serial.println(target_uids[i]);
    }
    Serial.println("---------------------------------");
}
void UWBManager::loop()
{
    switch (currentState)
    {
    case IDLE:
        break;
    case READY:
        if (thisDevice.type == TAG)
        {
            initiator();
            break;
        }
        else if (thisDevice.type == ANCHOR)
        {
            responder();
            break;
        }
        break;
    case CLUSTER_UPDATE:
        updateClusterFromServer();
        set_target_uids();
        if (thisDevice.type == TAG)
            dwt_setrxtimeout(RX_TIMEOUT_UUS);
        else
            dwt_setrxtimeout(0);
        dwt_configuretxrf(&txconfig_options);
        dwt_setrxantennadelay(RX_ANT_DLY);
        dwt_settxantennadelay(TX_ANT_DLY);
        dwt_setrxaftertxdelay(TX_TO_RX_DLY_UUS);
        tx_msg[MSG_SID_IDX] = thisDevice.UID;
        rx_msg[MSG_SID_IDX] = thisDevice.UID;
        dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);
        changeState(READY);
        break;
    default:
        break;
    }
}

float *UWBManager::getDistances()
{
    return distances;
}

// Wartet, bis eines der angegebenen RX-Ereignisse (Nachricht empfangen, Timeout, Fehler)
// auftritt, und gibt dann den Status zurück.
uint32_t UWBManager::waitForReceptionEvent(uint32_t eventMask)
{
    uint32_t status;
    while (!((status = dwt_read32bitreg(SYS_STATUS_ID)) & eventMask))
    {
        // Warten...
    }
    return status;
}
// Versendet eine Nachricht und wartet, bis die Übertragung abgeschlossen ist.
// Gibt true zurück, wenn der Sendevorgang erfolgreich war.
bool UWBManager::sendTxMessage(uint8_t *msg, uint16_t len, uint32_t txFlags)
{
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
    dwt_writetxdata(len, msg, 0);
    dwt_writetxfctrl(len, 0, 1);
    int ret = dwt_starttx(txFlags);
    if (ret == DWT_SUCCESS)
    {
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
        {
            // Warte auf Abschluss
        }
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
        return true;
    }
    return false;
}

// ===================== Initiator-spezifische Funktionen =====================

// Sendet eine Poll-Nachricht, um den Ranging-Vorgang zu starten.
void UWBManager::initiatorSendPoll()
{
    wait_ack = true;
    tx_msg[MSG_SN_IDX] = frame_seq_nb;
    tx_msg[MSG_FUNC_IDX] = FUNC_CODE_POLL;
    sendTxMessage(tx_msg, MSG_LEN, DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
}

// Sendet die Range-Nachricht, nachdem alle ACKs empfangen wurden.
void UWBManager::initiatorSendRange()
{
    // Berechne den verzögerten TX-Zeitpunkt
    tx_time = (get_rx_timestamp_u64() + (RX_TO_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
    tx_ts = (((uint64_t)(tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;
    dwt_setdelayedtrxtime(tx_time);
    tx_msg[MSG_SN_IDX] = frame_seq_nb;
    tx_msg[MSG_FUNC_IDX] = FUNC_CODE_RANGE;
    sendTxMessage(tx_msg, BUF_LEN, DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);
    wait_ack = false;
    wait_final = true;
    counter = 0;
}
// ===================== Responder-spezifische Funktionen =====================

// Sendet eine ACK-Nachricht als Antwort auf einen Poll.
void UWBManager::responderSendAck()
{
    tx_time = (get_rx_timestamp_u64() + (RX_TO_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
    tx_ts = (((uint64_t)(tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;
    dwt_setdelayedtrxtime(tx_time);
    tx_msg[MSG_SN_IDX] = frame_seq_nb;
    tx_msg[MSG_FUNC_IDX] = FUNC_CODE_ACK;
    // Setzt den Zeitstempel-Differenzwert in das ACK-Paket
    resp_msg_set_ts(&tx_msg[MSG_T_REPLY_IDX], tx_ts - poll_rx_ts);
    sendTxMessage(tx_msg, BUF_LEN, DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);
}

// Sendet eine Final-Nachricht als Antwort auf eine Range-Nachricht.
void UWBManager::responderSendFinal()
{
    ack_tx_ts = get_tx_timestamp_u64();
    tx_time = (get_rx_timestamp_u64() + (RX_TO_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
    tx_ts = (((uint64_t)(tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;
    dwt_setdelayedtrxtime(tx_time);
    // Setzt den Zeitstempel-Differenzwert in das Final-Paket
    resp_msg_set_ts(&tx_msg[MSG_T_REPLY_IDX], range_rx_ts - ack_tx_ts);
    tx_msg[MSG_SN_IDX] = frame_seq_nb;
    tx_msg[MSG_FUNC_IDX] = FUNC_CODE_FINAL;
    sendTxMessage(tx_msg, BUF_LEN, DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);
}
// Liest und verarbeitet eine empfangene Nachricht im Responder.
void UWBManager::processResponderMessage()
{
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
    dwt_readrxdata(rx_buffer, BUF_LEN, 0);

    // Bei einem Reset-Befehl: Setze den Poll-Modus und Zähler zurück
    if (rx_buffer[MSG_FUNC_IDX] == FUNC_CODE_RESET)
    {
        wait_poll = true;
        wait_range = false;
        counter = 0;
        return;
    }
    // Überprüfe, ob die Nachricht von dem erwarteten Absender stammt
    if (rx_buffer[MSG_SID_IDX] != target_uids[counter])
    {
        dwt_write32bitreg(SYS_STATUS_ID,
                          SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        wait_poll = true;
        wait_range = false;
        counter = 0;
        return;
    }
    // Verarbeite empfangene Nachrichten je nach aktuellem Modus:
    if (wait_poll)
    {
        if (counter == 0)
        {
            poll_rx_ts = get_rx_timestamp_u64();
        }
        ++counter;
    }
    else if (wait_range)
    {
        if (counter == 0)
        {
            range_rx_ts = get_rx_timestamp_u64();
        }
        ++counter;
    }
}
void UWBManager::clearCluster()
{
    currentCluster.name[0] = '\0';
    currentCluster.deviceCount = 0;
    for (uint8_t i = 0; i < MAX_DEVICES; i++)
    {
        currentCluster.devices[i] = nullptr;
    }
    for (uint8_t i = 0; i < MAX_DEVICES - 1; i++)
    {
        distances[i] = 0.0f;
        target_uids[i] = 0;
    }
}
void UWBManager::changeState(UWBState state)
{
    currentState = state;
}

bool UWBManager::getDistanceJson(JsonDocument &doc)
{
    if (currentState != READY)
    {
        return false;
    }
    char addrStr[13];
    snprintf(addrStr, sizeof(addrStr), "%012llX", thisDevice.address);

    doc["type"] = (thisDevice.type == TAG) ? "TAG" : "ANCHOR";
    doc["address"] = addrStr;

    JsonObject cluster = doc["cluster"].to<JsonObject>();
    cluster["name"] = currentCluster.name;

    JsonArray devices = cluster["devices"].to<JsonArray>();
    for (uint8_t i = 1; i < currentCluster.deviceCount; i++)
    {
        Node *dev = currentCluster.devices[i];
        char devAddrStr[13];
        snprintf(devAddrStr, sizeof(devAddrStr), "%012llX", dev->address);
        JsonObject devObj = devices.add<JsonObject>();
        devObj["address"] = devAddrStr;
        devObj["type"] = (dev->type == TAG) ? "TAG" : "ANCHOR";
        devObj["raw_distance"] = dev->raw_distance;
    }
    return true;
}
