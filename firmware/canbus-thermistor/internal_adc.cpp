#include <Arduino.h>
#include <ArduinoLog.h>
#include <canbus_ids.h>
#include <sensor.h>

#include "project.h"
#include "internal_adc.h"
#include "canbus.h"

void InternalADCSensor::init(void)
{
  int bits = -1;
  for (int i = 0; i < 3 && bits == -1; i++) {
    if (_bits_allowed[i] == _bits) {
      bits = _bits;      
    } 
  }

  if (bits && ((_channel >= 0 && _channel <= 8) || (_channel >= 16 && _channel <= 17))) {
    _valid = true;
  }

  if (!_valid){
    Log.error("InternalADC@%d not configured correctly", _channel);
    return;
  }

  Log.info("Setting up InternalADC@%d", _channel);

  // Set the resolution
  analogReadResolution(_bits);

  if (!_div) {
    _mult = 1;
    _div = 1;
  }

  // Read out the temperature sensor and vref calibration
  _ts_cal1 = (int32_t)*((uint16_t *)0x1FFFF7B8);
  _ts_cal2 = (int32_t)*((uint16_t *)0x1FFFF7C2);
  _vrefint_cal = (int32_t)(*(uint16_t *)0x1FFFF7BA);
}

int32_t InternalADCSensor::get_raw_value(void)
{
  if (!_valid) {
    return UNUSED_VALUE;
  }

#ifdef VERBOSE_LOGGING
  Log.notice("Reading Internal ADC Channel %d", _channel);
#endif

  int pin;

  switch (_channel) {
    case 16:
      pin = ATEMP;
      break;
    case 17:
      pin = AVREF;
      break;
    default:
      pin = analogInputPin[_channel];
      break;
  }

  return (int32_t)analogRead(pin);
}

int32_t InternalADCSensor::convert(int32_t reading)
{
  if (reading == UNUSED_VALUE) {
    return UNUSED_VALUE;
  }
  
  switch (_channel) {
    case 16:
      // internal temperature
      // Temperature is via linear regression mapped out by calibration registers
      // temp [C] = (TS_CAL2_TEMP - TS_CAL1_TEMP) / (TS_CAL2 - TS_CAL1) * (TS_DATA - TS_CAL1) + TS_CAL1_TEMP
      return map<int32_t>(reading, _ts_cal1, _ts_cal2, _ts_cal1_temp, _ts_cal2_temp, false);
    case 17:
      // internal vref
      if (reading == 0) {
        return UNUSED_VALUE;
      }
      return 3300 * _vrefint_cal / reading;
    default:
      break;
  }

  if (_vref == -1) {
    int32_t vref = analogRead(AVREF);
    if (vref == 0) {
      return UNUSED_VALUE;
    }
    _vref = 3300 * _vrefint_cal / vref;
  }

  reading = ((reading * _vref * _mult) / _div) >> _bits;    // will be in mV
  if (_convertfunc != 0) {
    reading = _convertfunc(reading);
  }
  return reading;
}

void InternalADCSensor::do_feedback(void)
{
  if (_id) { 
    canbus_output_value(_id, _value, _data_bytes);
  }
}
