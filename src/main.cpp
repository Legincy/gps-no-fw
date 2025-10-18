#include "Initiator.h"

void setup()
{
    Serial.begin(115200);
    if (!Initiator::getInstance().begin())
    {
        while (true)
            ;
    }
}

void loop()
{
    Initiator::getInstance().runTag();
}
