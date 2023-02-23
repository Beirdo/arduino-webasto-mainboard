#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "dummy.h"
#include "sensor_eeprom.h"
#include "analog.h"

const int32_t dummy_values[MAX_INDEX] = {
  -5000,    // External temp
  11768,    // Battery Voltage
  5000,     // Coolant temp
  -5000,    // Exhaust temp
  0,        // Ignition sense (off)
  0,        // Emergency stop (off)
  20000,    // Internal temp
  2000,     // Flame Detector
  0,        // Start/Run (off)
  4800,     // VSYS voltage,
  UNUSED_READING, // LINBus bridge,
};

void DummySource::init(void)
{
  Log.info("Substituted in a dummy %s sensor", capabilities_names[_index]);
  _value = dummy_values[_index];
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
