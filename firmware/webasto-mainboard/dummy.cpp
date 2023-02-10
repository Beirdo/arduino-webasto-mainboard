#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "dummy.h"
#include "sensor_eeprom.h"
#include "analog.h"

void DummySource::init(void)
{
  Log.info("Substituted in a dummy %s sensor", capabilities_names[_index]);
  if (_index == INDEX_EMERGENCY_STOP) {
    _value = 1;
  } else {
    _value = 0;
  }
  _connected = true;
}

int32_t DummySource::read_device(void)
{
  return _value;
}

int32_t DummySource::convert(int32_t reading)
{
  return reading;
}
