#include <Arduino.h>
#include <pico.h>

#include "pca9501.h"
#include "fsm.h"

void PCA9501DigitalSource::init(void)
{
  uint8_t config;
  _valid = true;
  i2c_read_data(0, &config, 1, true);

  // Make sure to set the quasi-bidirectional I/O to HIGH so we can use it as an input.
  config |= _bitmask;
}

int32_t PCA9501DigitalSource::read_device(void)
{
  uint8_t buf;

  i2c_read_data(0, &buf, 1, true);

  return (int32_t)(buf & _bitmask);
}

int32_t PCA9501DigitalSource::convert(int32_t reading)
{
  // Normalize to 0/1 regardless of bitmask
  return !(!reading);
}
