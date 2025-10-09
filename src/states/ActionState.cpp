#include "states/ActionState.h"

void ActionState::enter()
{
    log.info("ActionState", "Entering ActionState");
}

void ActionState::update()
{
    mqttManager.update();
    if (configManager.isDeviceTag())
    {
        if (uwbManager.performRangingCycleAndCreatePayload(&jsonDoc))
        {
            if (millis() - last_ranging_time >= RANGING_INTERVAL_MS)
            {
                String payload;
                serializeJson(jsonDoc, payload);
                MQTTManager::getInstance().publishMeasurement(payload.c_str());

                last_ranging_time = millis();
            }
        }
    }
    else
        uwbManager.responder_loop();
}

void ActionState::exit()
{
    log.info("ActionState", "Exiting ActionState");
}
