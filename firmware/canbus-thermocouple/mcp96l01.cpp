#include <Arduino.h>
#include <ArduinoLog.h>

#include "mcp96l01.h"

void MCP96L01Sensor::init(void)
{
  if (_bits >= _min_bits && _bits <= _max_bits && _type >= 0 && _type < TYPE_COUNT) {
    _valid = true;
  }

  if (!_valid){
    Log.error("MCP96L01@%X/I2C not configured correctly", _i2c_address);
    return;
  }

  Log.notice("Setting up MCP96L01@%X/I2C", _i2c_address);

  // Set the thermocouple type and filter coefficient bits
  i2c_write_register(0x05, ((_type << 4) & 0x70) | (_filter_bits & 0x07));

  // Set device configuration, use lower resolution cold junction readings for speed
  _device_config = 0;
  _device_config |= (1 << 7);   // cold junction resolution of 0.25C
  _device_config |= (((18 - _bits) & 0x06) << 4);  // device resolution - Th always reads back as 16bit

  // shutdown mode until I say otherwise, please.
  i2c_write_register(0x06, _device_config | 1);

  switch(_bits) {
    case 12:
      _conv_ms = 5;
      break;
    case 14:
      _conv_ms = 20;
      break;
    case 16:
      _conv_ms = 80;
      break;
    case 18:
      _conv_ms = 320;
      break;
    default:
      break;
  }
}

int32_t MCP96L01Sensor::get_raw_value(void)
{
  if (!_valid) {
    return UNUSED_VALUE;
  }

  // Turn this on, in one-shot mode
  i2c_write_register(0x06, _device_config);

  // Trigger a reading
  i2c_write_register(0x04, 0x40);

  delay(_conv_ms);

  // delay a bit longer if needed
  uint8_t status;
  while (true) {
    i2c_read_data(0x04, &status, 1);
    if (!(status & 0x04)) {
      break;
    }
    delayMicroseconds(500);
  }

  uint8_t raw_reading[2];
  // Read out the results
  i2c_read_data(0x00, raw_reading, 2);

  // Put back into power-down
  i2c_write_register(0x06, _device_config | 1);

  uint16_t reading = ((uint16_t)(raw_reading[0]) << 8) | raw_reading[1];
  int32_t retval = (int32_t)reading;
  return retval;
}

int32_t MCP96L01Sensor::convert(int32_t reading)
{
  // data is in S11.4, we want degC to 2 decimals
  reading *= 100;
  reading /= 16;
  return reading;
}
