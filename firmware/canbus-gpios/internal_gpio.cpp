#include <Arduino.h>
#include <ArduinoLog.h>
#include <canbus_ids.h>
#include <sensor.h>

#include "project.h"
#include "internal_gpio.h"
#include "canbus.h"

void InternalGPIOSensor::init(void)
{
  if (_pin == 0 || _pin == 1 || _pin == 4) {
    _valid = true;
  }

  if (!_valid){
    Log.error("InternalGPIO@%d not configured correctly", _pin);
    return;
  }

  Log.info("Setting up InternalGPIO@%d", _pin);

  pinMode(_pin, INPUT);  
}

int32_t InternalGPIOSensor::get_raw_value(void)
{
  if (!_valid) {
    return UNUSED_VALUE;
  }

#ifdef VERBOSE_LOGGING
  Log.notice("Reading Internal GPIO Pin %d", _pin);
#endif

  return (int32_t)digitalRead(_pin);
}

int32_t InternalGPIOSensor::convert(int32_t reading)
{
  if (reading == UNUSED_VALUE) {
    return UNUSED_VALUE;
  }
  
  return !(!reading);
}

void InternalGPIOSensor::do_feedback(void)
{
  if (_id) { 
    canbus_output_value(_id, _value, _data_bytes);
  }
}
