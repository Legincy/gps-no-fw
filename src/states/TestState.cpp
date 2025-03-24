#include "states/TestState.h"

void TestState::enter()
{
    log.info("TestState", "Entering TestState");

    if (!uwb.begin())
    {
        log.error("TestState", "Failed to initialize UWBManager");
    }

#ifdef NODE_TYPE
#if NODE_TYPE == 0
    uwb.setNodeType(NodeType::TAG);
#elif NODE_TYPE == 1
    uwb.setNodeType(NodeType::ANCHOR);
#else
    log.info("TestState", "NODE_TYPE is neither TAG nor ANCHOR");
    uwb.setNodeType(NodeType::TAG);
#endif
#endif
}

void TestState::update()
{
    uwb.printStatus();

    delay(1000);
}

void TestState::exit()
{
    log.info("TestState", "Exiting TestState");
}