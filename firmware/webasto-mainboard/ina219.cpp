#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "ina219.h"
#include "fsm.h"


void INA219Source::init(void)
{
  if (!_connected) {
    _valid = false;
    return;
  }

  if (_bits >= _min_bits && _bits <= _max_bits) {
    _valid = true;
  }

  if (!_valid){
    Log.error("INA219@0x%02X/I2C not configured correctly", _i2c_address);
    return;
  }

  Log.notice("Setting up INA219@0x%02X/I2C", _i2c_address);

  // Send the chip a reset, clearing all values to factory defaults
  i2c_write_register_word(0x00, 0x8000);

  // Now set it up the way we want it.  Use 16V range for bus measurement, +/-320mV on shunt, set the ADC bits size, power down
  _device_config = 0;
  _device_config |= (3 << 11);    // PG = 3, +/- 320mV
  _device_config |= ((-9 - _bits) & 0x03) << 7;   /* Bus ADC -> bits wide */
  _device_config |= ((-9 - _bits) & 0x03) << 3;   /* Shunt ADC -> bits wide */
  i2c_write_register_word(0x00, _device_config);  

  switch(_bits) {
    case 9:
      _conv_us = 84;
      break;
    case 10:
      _conv_us = 148;
      break;
    case 11:
      _conv_us = 276;
      break;
    case 12:
      _conv_us = 532;
      break;
    default:
      break;
  }
} 

int32_t INA219Source::read_device(void)
{
  if (!_valid) {
    return UNUSED_READING;
  }

  if (_enable_signal && !(*_enable_signal)) {
    // We are not currently enabled.
    return DISABLED_READING;
  }

  // Trigger a measurement of the shunt ADC
  i2c_write_register_word(0x00, _device_config | 0x0001);

  delayMicroseconds(_conv_us + 1);

  uint16_t status;
  while (true) {
    i2c_read_data(0x02, (uint8_t *)&status, 2);
    if (status & 0x0002) {
      break;
    }
    delayMicroseconds(20);
  }

  uint16_t raw_reading;
  i2c_read_data(0x01, (uint8_t *)&raw_reading, 2);

  // put it back in power down
  i2c_write_register_word(0x00, _device_config);

  int32_t retval = abs((int16_t)raw_reading);
  return retval;    // shunt voltage in mV (absolute value, should always be positive!)
}

int32_t INA219Source::convert(int32_t reading)
{
  // Apply Ohms law to find resistance
  int32_t current = 100;      // 100mA
  int32_t voltage = reading;  // in mV

  int32_t resistance = voltage * 1000 / current;    // we want milli-ohm
  return resistance;
}

void INA219Source::feedback(int index)
{
  if (_prev_value == UNUSED_READING || abs(_prev_value - _value) > 20) {
    if (index == 6) {
      FlameDetectEvent event;
      event.value = (int)_value;
      WebastoControlFSM::dispatch(event);
    }
  }
}
