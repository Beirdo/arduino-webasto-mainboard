#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>
#include <canbus_ids.h>

#include "project.h"
#include "internal_adc.h"
#include "fsm.h"
#include "canbus.h"

void InternalADCSensor::init(void)
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

int32_t InternalADCSensor::get_raw_value(void)
{
  if (!_valid) {
    return UNUSED_VALUE;
  }

#ifdef VERBOSE_LOGGING
  Log.notice("Reading Internal ADC Channel %d", _channel);
#endif
  if (_channel == 4) {
    float vref = mainboardDetected ? 3.3 : 3.3; //3.0 for onboard once I put parts on there;
    float temp = analogReadTemp(vref);
    return (int32_t)(temp * 100.0);
  }

  return (int32_t)analogRead(26 + _channel);
}

int32_t InternalADCSensor::convert(int32_t reading)
{
  if (_channel == 2) {
    // Wired to VSYS / 3 on our board
    int32_t vref = mainboardDetected ? 3300 : 3300; // 3000 once onboard connected
    int32_t retval = (reading * vref * 3) >> _bits;
#ifdef VERBOSE_LOGGING
    Log.notice("VSYS = %dmV", retval);
#endif
    return retval;
  }

  if (_channel == 4) {
    // Already formatted this as part of conversion from float
    return reading;
  }

  return LocalSensor::convert(reading);
}

void InternalADCSensor::do_feedback(void)
{ 
  canbus_output_value(_id, _value, _data_bytes);
  
  switch (_id) {
    case CANBUS_ID_INTERNAL_TEMP:
      {
        InternalTempEvent event;
        event.value = (int)_value;
        WebastoControlFSM::dispatch(event);
      }
      break;

    case CANBUS_ID_VSYS_VOLTAGE:
      {
        VSYSLevelEvent event;
        event.value = _value;
        WebastoControlFSM::dispatch(event);
      }
      break;

    default:
      break;
  }  
}
