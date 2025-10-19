#include <Arduino.h>
#include <Wire.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_core_dump.h"
#include "managers/ConfigManager.h"
#include "managers/LogManager.h"
#include "states/IdleState.h"
#include "states/TestState.h"
#include "Device.h"

void setup()
{
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  esp_core_dump_init();

  Serial.begin(MONITOR_SPEED);
  Serial.println(F("###################################################"));
  Serial.println(F("(c) 2023-2024 Hochschule Bochum GPS:NO - Martin Peth, Niklas Schütrumpf, Felix Schwarz"));
  Serial.printf("Compiled with c++ version %s", __VERSION__);
  Serial.println();
  Serial.printf("Version v--- @--- %s at %s", __DATE__, __TIME__);
  Serial.println();
  Serial.println(F("###################################################"));

  Device &device = Device::getInstance();

  if (!device.begin())
  {
    Serial.println(F("Failed to initialize device"));
    while (true)
      ;
  }

  device.changeState(IdleState::getInstance(&device));
}

void loop()
{
  Device::getInstance().update();
}
