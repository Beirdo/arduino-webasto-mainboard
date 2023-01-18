#include <Arduino.h>
#include <pico.h>

#include "internal_adc.h"

void InternalADCSource::init(void)
{
  if (_bits >= _min_bits && _bits <= _max_bits && _channel >= 0 && _channel < 4) {
    _valid = true;
  }

  if (!_valid){
    // logmsg("InternalADC@%d not configured correctly", _channel");
    return;
  }

  // Set the resolution
  analogReadResolution(_bits);
}

int32_t InternalADCSource::read_device(void)
{
  if (_channel == 3) {
    float temp = analogReadTemp();
    return (int32_t)(temp * 100.0);
  }

  return (int32_t)analogRead(26 + _channel);
}

int32_t InternalADCSource::convert(int32_t reading)
{
  if (_channel == 3) {
    // Already formatted this as part of conversion from float
    return reading;
  }

  return AnalogSourceBase::convert(reading);
}
