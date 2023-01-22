#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "ads7823.h"
#include "fsm.h"

void ADS7823Source::init(void)
{
  if (_bits >= _min_bits && _bits <= _max_bits) {
    _valid = true;
  }

  if (!_valid) {
    Log.error("ADS7823@0x%02X/I2C is not configured correctly", _i2c_address);
    return;
  }

  Log.notice("Setting up ADS7823@0x%02X/I2C", _i2c_address);

  // This ADC doesn't even have a control register.  Cool.
}

int32_t ADS7823Source::read_device(void)
{
  if (!_valid) {
    return UNUSED_READING;
  }
  
  uint16_t raw_readings[4];
  i2c_read_data(0x00, (uint8_t *)&raw_readings, sizeof(raw_readings));

  int32_t accumulator = 0;
  for (int i = 0; i < 4; i++) 
  {
    accumulator += raw_readings[i];
  }

  return accumulator / 4;
}

void ADS7823Source::feedback(int index)
{
  if (_prev_value == UNUSED_READING || abs(_prev_value - _value) > 100) {
    if (index == 1) {
      BatteryLevelEvent event;
      event.value = _value;
      WebastoControlFSM::dispatch(event);
    }
  }
}
