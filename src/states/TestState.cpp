#include "states/TestState.h"

void TestState::enter()
{
    log.info("TestState", "Entering TestState");

    if (!uwb.begin())
    {
        log.error("TestState", "Failed to initialize UWBManager");
    }
    uwb.changeState(CLUSTER_UPDATE);
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