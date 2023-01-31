#include <Arduino.h>
#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>

#include "analog_source.h"

AnalogSourceBase::AnalogSourceBase(uint8_t i2c_address, int bits, int count, int mult, int div_)
{
  _valid = false;
  _i2c_address = i2c_address;
  _bits = bits;
  _bytes = (_bits >> 8) + ((_bits & 0x07) != 0);
  
  _reading_count = max(count, 1);
  _data = new readings_t[_reading_count];

  for (int i = 0; i < _reading_count; i++) {
    for (int j = 0; j < ADC_AVG_WINDOW; j++) {
      _data[i].readings[j] = UNUSED_READING;
    }
    _data[i].tail = 0;
    _data[i].prev_value = UNUSED_READING;
    _data[i].value = UNUSED_READING;
  }
  
  _mult = mult;
  _div = div_;
  
  mutex_init(&_i2c_mutex);

  if (_i2c_address == 0xFF) {
    _connected = false;
  } else if (_i2c_address == 0x00) {
    _connected = true;
  } else {
    _connected = i2c_is_connected();
  }

  if (_connected) {
    Log.notice("Found sensor at I2C %X", _i2c_address);
  } else {
    Log.error("No sensor at I2C %X", _i2c_address);
  }
}

AnalogSourceBase::~AnalogSourceBase(void)
{
  delete [] _data;
}

void AnalogSourceBase::update(void)
{
  int32_t scaled_value;
  
  if (!_connected) {
    return;
  }

  read_device();

  for (int i = 0; i < _reading_count; i++) {
    scaled_value = convert(i);
    append_value(i, scaled_value);
    _data[i].value = filter(i);
  }
}

int32_t AnalogSourceBase::filter(int index)
{
  int32_t accumulator = 0;  
  int count = 0;
  int32_t min_reading = (int32_t)0x7FFFFFFF;
  int min_index = -1;
  int32_t max_reading = (int32_t)0x80000000;
  int max_index = -1;
  int32_t value;

  for (int i = 0; i < ADC_AVG_WINDOW; i++) {
    value = _data[index].readings[i];
    if (value != UNUSED_READING && value < min_reading) {
      min_reading = value;
      min_index = i;
    }

    if (value != UNUSED_READING && value > max_reading) {
      max_reading = value;
      max_index = i;
    }
  }

  for (int i = 0; i < ADC_AVG_WINDOW; i++) {
    value = _data[index].readings[i];
    if (value == UNUSED_READING) {
      continue;            
    }
    
    if (i == min_index) {
      continue;
    }    

    if (i == max_index) {
      continue;
    }

    accumulator += value;
    count++;
  }

  if (!count) {
    return 0;
  }

  return accumulator / count;
}

void AnalogSourceBase::append_value(int index, int32_t value)
{
  if (value == UNUSED_READING || value == DISABLED_READING) {
    return;
  }  
  
  int tail = _data[index].tail;
  _data[index].readings[tail] = value;
  tail += 1;
  tail %= ADC_AVG_WINDOW;
  _data[index].tail = tail;
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

int32_t AnalogSourceBase::convert(int index)
{
  int32_t value = 0;

  if (_div == 0) {
    return 0;
  }

  value = _data[index].raw_value;
  value *= _mult;
  value /= _div;
  return value;
}
