#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "dummy.h"
#include "sensor_eeprom.h"

void DummySource::init(void)
{
  Log.info("Substituted in a dummy %s sensor", capabilities_names[_index]);
  _connected = true;
}

void DummySource::feedback(int index)
{
  // Sorry, FSM...  this sensor is missing
}

int32_t DummySource::read_device(void)
{
  return 0;
}

int32_t DummySource::convert(int32_t reading)
{
  (void)reading;
  return 0;
}
