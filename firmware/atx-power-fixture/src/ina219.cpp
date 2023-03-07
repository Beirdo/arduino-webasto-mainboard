#include <Arduino.h>
#include <ArduinoLog.h>

#include "ina219.h"


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
    Log.error("INA219@%X/I2C not configured correctly", _i2c_address);
    return;
  }

  Log.notice("Setting up INA219@%X/I2C", _i2c_address);

  // Send the chip a reset, clearing all values to factory defaults
  i2c_write_register_word(0x00, 0x8000);

  // Now set it up the way we want it.  Use 16V range for bus measurement, +/-320mV on shunt, set the ADC bits size, power down
  _device_config = 0;
  _device_config |= (3 << 11);    // PG = 3, +/- 320mV
  _device_config |= ((-9 - _bits) & 0x03) << 7;   /* Bus ADC -> bits wide */
  _device_config |= ((-9 - _bits) & 0x03) << 3;   /* Shunt ADC -> bits wide */
  i2c_write_register_word(0x00, _device_config);  

  // calibration = 0.04096 / (Current LSB * Rshunt [Ohms])
  //             = 40.96 / (Current LSB * Rshunt [mOhms])
  //             = 40.96 / ((Max Current [A] / 2**15) * Rshunt [mOhms])
  //             = 40960 / ((Max Current [mA] / 2**15) * Rshunt [mOhms])
  //             = 40960 * 2**15 / Max Current [mA] / Rshunt [mOhms]
  //             = 1342177280 / Max Current [mA] / Rshunt [mOhms]
  int calibration = 1342177280 / _max_current / _rshunt;

  i2c_write_register_word(0x05, ((uint16_t)calibration) & 0xFFFE);

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

void INA219Source::read_device(void)
{
  if (!_valid) {
    for (int i = 0; i < _reading_count; i++) {
      _data[i].raw_value = UNUSED_READING;
    }
    return;
  }

  // Trigger a measurement of the shunt and bus ADC
  i2c_write_register_word(0x00, _device_config | 0x0003);

  delayMicroseconds(_conv_us + 1);

  uint16_t status;
  while (true) {
    i2c_read_data(0x02, (uint8_t *)&status, 2);
    if (status & 0x0002) {
      break;
    }
    delayMicroseconds(20);
  }

  uint16_t raw_reading[4];
  i2c_read_data(0x01, (uint8_t *)&raw_reading, 8);

  // put it back in power down
  i2c_write_register_word(0x00, _device_config);

  for (int i = 0; i < _reading_count; i++) {
    _data[i].raw_value = raw_reading[i];
  }
}

int32_t INA219Source::convert(int index)
{
  int raw_reading = _data[index].raw_value;
  if (raw_reading == UNUSED_READING || raw_reading == DISABLED_READING) {
    return raw_reading;
  }  

  switch(index) {
    case 0:
      // shunt voltage - we get it as centivolts.   make it millivolts
      return raw_reading * 10;                          // [mV]

    case 1:
      // bus voltage - mask off status bits and multiply by LSB of 4mV
      return (raw_reading & 0xFFF8) * 4;                // [mV]
                   
    case 2:
      // power register - power LSB = 20 * current LSB
      return (raw_reading * 20 * _max_current) >> 15;   // [mW]

    case 3:
      // current register
      return (raw_reading * _max_current) >> 15;        // [mA]

    default:
      return UNUSED_READING;
  }
}
