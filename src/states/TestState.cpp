#include "states/TestState.h"

void TestState::enter()
{
    log.info("TestState", "Entering TestState");
}

void TestState::update()
{
    // test Code
}

void TestState::exit()
{
    log.info("TestState", "Exiting TestState");
}