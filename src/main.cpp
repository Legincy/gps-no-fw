#include "Device.h"

void setup()
{
    Serial.begin(115200);
    Device &device = Device::getInstance();
    device.begin();
}

void loop()
{
    Device &device = Device::getInstance();
    device.update();
}
