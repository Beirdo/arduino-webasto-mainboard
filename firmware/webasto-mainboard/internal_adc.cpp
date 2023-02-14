#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "project.h"
#include "internal_adc.h"

void InternalADCSource::init(void)
{
  if (_bits >= _min_bits && _bits <= _max_bits && _channel >= 0 && _channel <= 4) {
    _valid = true;
  }

  if (!_valid){
    Log.error("InternalADC@%d not configured correctly", _channel);
    return;
  }

  Log.info("Setting up InternalADC@%d", _channel);

  // Set the resolution
  analogReadResolution(_bits);
}

int32_t InternalADCSource::read_device(void)
{
  if (!_valid) {
    return 0;
  }

#ifdef VERBOSE_LOGGING
  Log.notice("Reading Internal ADC Channel %d", _channel);
#endif
  if (_channel == 4) {
    float vref = mainboardDetected ? 3.3 : 3.3; //3.0 for onboard once I put parts on there;
    float temp = analogReadTemp(vref);
    return (int32_t)(temp * 100.0);
  }

  if (_enable_pin == -1 || digitalRead(_enable_pin)) {
    return (int32_t)analogRead(26 + _channel);
  } else {
    // Log.warning("Skipping reading ADC %d - gated off", _channel);
    return UNUSED_READING;
  }
}

int32_t InternalADCSource::convert(int32_t reading)
{
  if (_channel == 3) {
    // Wired to VSYS / 3 on the pico board itself
    int vref = mainboardDetected ? 3300 : 3300; // 3000 once onboard connected
    int retval = (reading * vref * 3) / (1 << _bits);
#ifdef VERBOSE_LOGGING
    Log.notice("VSYS = %dmV", retval);
#endif
    return retval;
  }

  if (_channel == 4) {
    // Already formatted this as part of conversion from float
    return reading;
  }

  return AnalogSourceBase::convert(reading);
}
