#include <Arduino.h>
#include <pico.h>

#include "internal_gpio.h"
#include "fsm.h"


void InternalGPIODigitalSensor::init(void)
{
  _valid = true;

  pinMode(_pin, INPUT);
}

int32_t InternalGPIODigitalSensor::get_raw_value(void)
{
  return digitalRead(_pin);
}

int32_t InternalGPIODigitalSensor::convert(int32_t reading)
{
  // Normalize to 0/1 regardless of bitmask
  if (_active_low) {
    return !reading;
  } else {
    return !(!reading);
  }
}
