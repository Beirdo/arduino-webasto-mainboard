#include <Arduino.h>
#include <pico.h>

#include "internal_gpio.h"
#include "fsm.h"


void InternalGPIODigitalSource::init(void)
{
  _valid = true;

  pinMode(_pin, INPUT);
}

int32_t InternalGPIODigitalSource::read_device(void)
{
  if (mainboardDetected) {
    return digitalRead(_pin);
  } else if (_active_low) {
    return 1;
  } else {
    return 0;
  }
}

int32_t InternalGPIODigitalSource::convert(int32_t reading)
{
  // Normalize to 0/1 regardless of bitmask
  if (_active_low) {
    return !reading;
  } else {
    return !(!reading);
  }
}
