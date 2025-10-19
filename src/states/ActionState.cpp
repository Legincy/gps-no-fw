#include "states/ActionState.h"

void ActionState::enter()
{
    log.info("ActionState", "Entering ActionState");
}

void ActionState::update()
{
    MQTTManager::getInstance().update();
}

void ActionState::exit()
{
    log.info("ActionState", "Exiting ActionState");
}
