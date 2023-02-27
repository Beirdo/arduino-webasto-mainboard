#include <Arduino.h>
#include <Wire.h>

#include "project.h"
#include "sensor.h"
#include "linbus_registers.h"

void LINBusRegister::init(void)
{
  if (_last_values) {
    delete [] _last_values;
  }

  _last_values = new uint8_t[_max_register()];
}

void LINBusRegister::update(void)
{
  int max_index = _max_register();
  uint16_t value;

  reset_read_addr(0);
  for (int i = 0; i < max_index; i += 2) {
    value = read_register_pair(i);
    _last_values[i] = HI_BYTE(value);
    if (i + 1 < max_index) {
      _last_values[i + 1] = LO_BYTE(value);
    }
  }

  _last_millis = millis();
}

void LINBusRegister::reset_read_addr(uint8_t new_addr)
{
  _read_addr = new_addr;
  Wire.beginTransmission(addr_linbus_bridge);
  Wire.write(_ident);
  Wire.write(_read_addr);
  Wire.endTransmission();
}

uint16_t LINBusRegister::read_register_pair(uint8_t addr)
{
  uint16_t value = 0;
  Wire.beginTransmission(addr_linbus_bridge);
  Wire.write(_ident);
  Wire.endTransmission(false);
  Wire.requestFrom(addr_linbus_bridge, 2);
  value |= (uint16_t)Wire.read() << 8;
  value |= Wire.read();

  return value;
}

void LINBusRegister::feedback(void)
{
  // Nothing to do here yet
}

int32_t LINBusRegister::get_value(uint8_t index)
{
  int32_t value = UNUSED_VALUE;
  if (index >= _max_register()) {
    return value;
  }

  uint16_t index_bit = BIT(index);
  if (width_8u & index_bit) {
    value = get8u(index);
  } else if (width_8s & index_bit) {
    value = get8s(index);
  } else if (width_16u & index_bit) {
    value = get16u(index);
  } else if (width_16s & index_bit) {
    value = get16s(index);
  }

  return value;
}

uint8_t LINBusRegister::get8u(uint8_t index)
{
  int age = millis() - _last_millis;
  if (age >= 1000) {
    update();
  }
  return _last_values[index];
}

int8_t LINBusRegister::get8s(uint8_t index)
{
  return (int8_t)get8u(index);
}

uint16_t LINBusRegister::get16u(uint8_t index)
{
  int age = millis() - _last_millis;
  if (age >= 1000) {
    update();
  }
  return ((uint16_t)_last_values[index] << 8) | (uint16_t)_last_values[index + 1];
}

int16_t LINBusRegister::get16s(uint8_t index)
{
  return (int16_t)get16u(index);
}

void LINBusRegister::write_register(uint8_t index, uint8_t value)
{
  _last_millis = 0;  // Force an update, no caching

  Wire.beginTransmission(addr_linbus_bridge);
  Wire.write(_ident);
  Wire.write(index);
  Wire.write(value);
  Wire.endTransmission();
}
