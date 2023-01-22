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
  int value = digitalRead(PIN_START_RUN);
  return value;  
}

int32_t InternalGPIODigitalSource::convert(int32_t reading)
{
  // Normalize to 0/1 regardless of bitmask
  return !(!reading);
}
  
void InternalGPIODigitalSource::feedback(int index)
{
  if (_prev_value != _value) {
    if (index == 7) {
      StartRunEvent event;
      event.enable = (bool)_value;
      WebastoControlFSM::dispatch(event);
    }
  }
}
