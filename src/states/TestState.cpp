#include "states/TestState.h"

void TestState::enter()
{
    log.info("TestState", "Entering TestState");

    if (!uwb.init())
    {
        log.error("TestState", "Failed to initialize UWBManager");
    }
    uwb.changeState(UWB_SETUP);
}

void TestState::update()
{
    // uwb.printStatus();
    uwb.loop();
    // delay(1000);
}

void TestState::exit()
{
    log.info("TestState", "Exiting TestState");
}