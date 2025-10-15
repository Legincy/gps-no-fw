#include "states/ActionState.h"

void ActionState::enter()
{
    log.info("ActionState", "Entering ActionState");
}

void ActionState::update()
{
    mqttManager.update();
    uwbManager.loop();
}

void ActionState::exit()
{
    log.info("ActionState", "Exiting ActionState");
}
