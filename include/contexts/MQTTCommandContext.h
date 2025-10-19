#ifndef MQTT_COMMAND_CONTEXT_H
#define MQTT_COMMAND_CONTEXT_H

#include "managers/MQTTManager.h"
#include "interfaces/ICommandContext.h"

class MQTTCommandContext : public ICommandContext
{
public:
    MQTTCommandContext()
        : mqttManager(MQTTManager::getInstance())
    {
    }

    void sendResponse(const char *response)
    {
        DynamicJsonDocument doc(2048);
        doc["type"] = "info";
        doc["source"] = "device";
        doc["payload"] = response;

        String payload;
        serializeJson(doc, payload);

        mqttManager.publish("response", payload.c_str());
        mqttManager.publish("response_raw", response);
    }

    void setResponseTopic(const char *topic)
    {
        responseTopic = topic;
    }

private:
    MQTTManager &mqttManager;
    String responseTopic;
};

#endif