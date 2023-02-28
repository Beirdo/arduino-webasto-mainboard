#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>

#include "ds2482.h"
#include "sensor.h"

void DS2482Sensor::wait_for_1wire(uint32_t poll_us) {
  uint8_t status_reg;

  while (true) {
    i2c_read_data(0, &status_reg, 1, true);
    if (!(status_reg & 0x01)) {
      return;
    }

    delayMicroseconds(poll_us);
  }
}

void DS2482Sensor::init(void)
{
  if (_bits >= _min_bits && _bits <= _max_bits) {
    _valid = true;
  }

  if (!_valid){
    Log.error("DS18B20/DS2482@%X/I2C not configured correctly", _i2c_address);
    return;
  }

  Log.notice("Setting up DS18B20/DS2482@%X/I2C", _i2c_address);

  // Reset the DS2482 I2c to 1Wire bridge
  i2c_write_register(0xF0, 0, true);

  // Initialize the DS2482 I2C to 1Wire bridge - APU on, SPU off (until needed), 1WS at standard speed
  i2c_write_register(0xD2, 0xE1);

  // Get and record the ROM ID of the one DS18B20 installed (for outside temperature)
  // Initialize 1Wire transaction with reset
  i2c_write_register(0xB4, 0, true);
  wait_for_1wire(500);

  // Send the Read Rom Command to the DS18B20
  i2c_write_register(0xA5, 0x33);
  wait_for_1wire(500);

  uint8_t *buf = _rom;
  for (int i = 0; i < 8; i++) {
    i2c_write_register(0x96, 0, true);
    wait_for_1wire(500);

    // Change read pointer, then read
    i2c_write_register(0xE1, 0xE1);
    i2c_read_data(0, buf++, 1, true);
  }

  // Initialize 1Wire transaction with reset
  i2c_write_register(0xB4, 0, true);
  wait_for_1wire(500);

  // Setup DS18B20 to do the correct number of bits as chosen by the user
  i2c_write_register(0xA5, 0x4E);
  wait_for_1wire(500);

  // Writing 3bytes of scratchpad
  i2c_write_register(0xA5, 0);
  wait_for_1wire(500);

  i2c_write_register(0xA5, 0);
  wait_for_1wire(500);

  i2c_write_register(0xA5, ((_bits - _min_bits) << 5) | 0x1F);
  wait_for_1wire(500);

  // Setup the delay from number of bits
  switch(_bits) {
    case 9:
      _conv_ms = 94;
      break;
    case 10:
      _conv_ms = 188;
      break;
    case 11:
      _conv_ms = 375;
      break;
    case 12:
      _conv_ms = 750;
      break;
    default:
      _conv_ms = 0;
      break;
  }
}

int32_t DS2482Sensor::get_raw_value(void)
{
  if (!_valid) {
    return UNUSED_VALUE;
  }

  // Initialize 1Wire transaction with reset
  i2c_write_register(0xB4, 0, true);
  wait_for_1wire(500);

  // Configure DS2482 to turn on the extra pullup immediately before sending a temperature command
  i2c_write_register(0xD2, 0xA5);
  i2c_write_register(0xA5, 0x44);

  delay(_conv_ms + 1);

  // Initialize 1Wire transaction with reset
  i2c_write_register(0xB4, 0, true);
  wait_for_1wire(500);

  // Read back first two scratchpad values
  i2c_write_register(0xA5, 0xBE);
  wait_for_1wire(500);

  uint8_t raw_reading[2];
  for (int i = 0; i < 2; i++) {
    i2c_read_data(0, &raw_reading[i], 1, true);
    wait_for_1wire(500);
  }

  // Abort 1Wire transaction with reset
  i2c_write_register(0xB4, 0, true);
  wait_for_1wire(500);

  uint16_t reading = ((uint16_t)raw_reading[0] << 8) | raw_reading[1];
  int16_t value = (int16_t)reading;
  bool negative = value < 0;

  if (negative) {
    value = -value;
  }

  value &= (0xFFFFFFFF << (12 - _bits));

  if (negative) {
    value = -value;
  }

  return value;
}

int32_t DS2482Sensor::convert(int32_t reading)
{
  if (reading != UNUSED_VALUE) {
    // comes in as S7.4, and we want this in degC with two decimals
    reading *= 100;
    reading >>= 4;
  }

  return reading;
}
