#include "states/IdleState.h"

void IdleState::enter()
{
    log.info("IdleState", "Entering IdleState");
}

void IdleState::update()
{
    device->changeState(TestState::getInstance(device));
}

void IdleState::exit()
{
    log.info("IdleState", "Exiting IdleState");
}