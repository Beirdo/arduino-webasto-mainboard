#include <Arduino.h>
#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>
#include <cxxabi.h>

#include "analog.h"
#include "analog_source.h"
#include "sensor_eeprom.h"
#include "fsm.h"

AnalogSourceBase::AnalogSourceBase(int index, int feedback_threshold, uint8_t i2c_address, int bits, int mult, int div_)
{
  _index = index;
  _valid = false;
  _i2c_address = i2c_address;
  _bits = bits;
  _bytes = (_bits >> 8) + ((_bits & 0x07) != 0);
  
  for (int i = 0; i < ADC_AVG_WINDOW; i++) {
    _readings[i] = UNUSED_READING;
  }
  
  _tail = 0;
  _value = UNUSED_READING;
  _mult = mult;
  _div = div_;
  _prev_value = UNUSED_READING;
  _feedback_threshold = feedback_threshold;
  
  mutex_init(&_i2c_mutex);

  if (_i2c_address == 0xFF) {
    _connected = false;
  } else if (_i2c_address == 0x00) {
    _connected = true;
  } else {
    _connected = i2c_is_connected();
  }

  if (_connected) {
    Log.notice("Found sensor (%s) at I2C %X", capabilities_names[_index], _i2c_address);
  } else {
    Log.error("No sensor (%s) at I2C %X", capabilities_names[_index], _i2c_address);
  }
}

void AnalogSourceBase::update(void)
{
  uint32_t raw_value;
  int32_t scaled_value;
  
  if (!_connected) {
    return;
  }

  raw_value = read_device();
  scaled_value = convert(raw_value);
  append_value(scaled_value);
  _value = filter();
}

int32_t AnalogSourceBase::filter(void)
{
  int32_t accumulator = 0;  
  int count = 0;
  int32_t min_reading = (int32_t)0x7FFFFFFF;
  int min_index = -1;
  int32_t max_reading = (int32_t)0x80000000;
  int max_index = -1;
  int32_t value;

  for (int i = 0; i < ADC_AVG_WINDOW; i++) {
    value = _readings[i];
    if (value != UNUSED_READING && value < min_reading) {
      min_reading = value;
      min_index = i;
    }

    if (value != UNUSED_READING && value > max_reading) {
      max_reading = value;
      max_index = i;
    }
  }

#ifdef VERBOSE_LOGGING
  if (min_index != -1) {
    Log.notice("Min: %d -> %d", min_index, _readings[min_index]);
  }

  if (max_index != -1) {
    Log.notice("Max: %d -> %d", max_index, _readings[max_index]);
  }
#endif

  for (int i = 0; i < ADC_AVG_WINDOW; i++) {
    value = _readings[i];
#ifdef VERBOSE_LOGGING
    Log.notice("Reading %d: %d", i, value);
#endif
    if (value == UNUSED_READING) {
      continue;            
    }
    
    if (i == min_index) {
#ifdef VERBOSE_LOGGING
      Log.notice("Discarding min: %d -> %d", i, value);
#endif
      continue;
    }    

    if (i == max_index) {
#ifdef VERBOSE_LOGGING
      Log.notice("Discarding max: %d -> %d", i, value);
#endif
      continue;
    }

    accumulator += value;
    count++;
  }

#ifdef VERBOSE_LOGGING
  Log.notice("Accumulator: %d, Count: %d", accumulator, count);  
#endif

  if (!count) {
    return 0;
  }

  return accumulator / count;
}

void AnalogSourceBase::append_value(int32_t value)
{
  if (value == UNUSED_READING || value == DISABLED_READING) {
    return;
  }  
  
  _readings[_tail] = value;
  _tail += 1;
  _tail %= ADC_AVG_WINDOW;
}

void AnalogSourceBase::i2c_write_register(uint8_t regnum, uint8_t value, bool skip_byte)
{
  CoreMutex m(&_i2c_mutex);
    
  Wire.beginTransmission(_i2c_address);
  Wire.write(regnum);
  if (!skip_byte) {
    Wire.write(value);
  }
  Wire.endTransmission();  
}

void AnalogSourceBase::i2c_write_register_word(uint8_t regnum, uint16_t value)
{
  CoreMutex m(&_i2c_mutex);
    
  Wire.beginTransmission(_i2c_address);
  Wire.write(regnum);
  Wire.write((value >> 8) & 0xFF);
  Wire.write(value & 0xFF);
  Wire.endTransmission();  
}

void AnalogSourceBase::i2c_read_data(uint8_t regnum, uint8_t *buf, uint8_t count, bool skip_regnum)
{
  CoreMutex m(&_i2c_mutex);

  Wire.beginTransmission(_i2c_address);
  if (!skip_regnum) {
    Wire.write(regnum);
    Wire.endTransmission(false);
  }
  Wire.requestFrom(_i2c_address, count);

  for (int i = 0; i < count && Wire.available(); i++) {
    *(buf++) = Wire.read();
  }
}

bool AnalogSourceBase::i2c_is_connected(void)
{
  CoreMutex m(&_i2c_mutex);

  Wire.beginTransmission(_i2c_address);
  return (Wire.endTransmission() == 0);
}

int32_t AnalogSourceBase::convert(int32_t reading)
{
  int32_t value = 0;

  if (_div == 0) {
    return 0;
  }

  value = reading;
  value *= _mult;
  value /= _div;
  return value;
}


void AnalogSourceBase::feedback(int index)
{
  if (!fsm_init) {
    return;
  }

  if (_prev_value == UNUSED_READING || abs(_prev_value - _value) > _feedback_threshold) {
    switch(index) {
      case INDEX_EXTERNAL_TEMP:
        {
          OutdoorTempEvent event;
          event.value = (int)_value;
          WebastoControlFSM::dispatch(event);
        }
        break;
      case INDEX_BATTERY_VOLTAGE:
        {
          BatteryLevelEvent event;
          event.value = _value;
          WebastoControlFSM::dispatch(event);
        }
        break;
      case INDEX_COOLANT_TEMP:
        {
          CoolantTempEvent event;
          event.value = (int)_value;
          WebastoControlFSM::dispatch(event);    
        }
        break;
      case INDEX_EXHAUST_TEMP:
        {
          ExhaustTempEvent event;
          event.value = (int)_value;
          WebastoControlFSM::dispatch(event);
        }
        break;
      case INDEX_IGNITION_SENSE:
        {        
          IgnitionEvent event;
          event.enable = (bool)_value;
          WebastoControlFSM::dispatch(event);
        }
        break;
      case INDEX_INTERNAL_TEMP:
        {
          InternalTempEvent event;
          event.value = (int)_value;
          WebastoControlFSM::dispatch(event);
        }
        break;
      case INDEX_FLAME_DETECTOR:
        {
          FlameDetectEvent event;
          event.value = (int)_value;
          WebastoControlFSM::dispatch(event);
        }
        break;
      case INDEX_START_RUN:
        {
          StartRunEvent event;
          event.enable = (bool)_value;
          WebastoControlFSM::dispatch(event);
        }
        break;
      case INDEX_EMERGENCY_STOP:
        {
          EmergencyStopEvent event;
          event.enable = (bool)_value;
          WebastoControlFSM::dispatch(event);
        }
        break;
      default:
        break;
    }
    _prev_value = _value;
 }
}
