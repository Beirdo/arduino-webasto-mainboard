#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "ads7823.h"
#include "analog.h"
#include "ntc_thermistor.h"

const int coolant_ntc_resistance_table[] = {
  669745, 623986, 581649, 542458, 506159, 472520, 441331, 412398, 385544, 360608,   // -50 - -41
  337440, 315905, 295877, 277243, 259897, 243743, 228691, 214660, 201574, 189365,   // -40 - -31
  177969, 167327, 157384, 148091, 139402, 131273, 123666, 116544, 109874, 103624,   // -30 - -21
  97765, 92271, 87118, 82281, 77741, 73476, 69470, 65704, 62164, 58834,             // -20 - -11
  55700, 52751, 49975, 47359, 44895, 42572, 40382, 38317, 36368, 34529,             // -10 - -1
  32791, 31165, 29628, 28176, 26802, 25504, 24275, 23111, 22010, 20968,             // 0 - 9
  19980, 19044, 18157, 17315, 16518, 15761, 15043, 14361, 13714, 13099,             // 10 - 19
  12515, 11960, 11432, 10931, 10454, 10000, 9568, 9157, 8766, 8393,                 // 20 - 29
  8038, 7700, 7378, 7071, 6778, 6498, 6232, 5978, 5735, 5503,                       // 30 - 39
  5282, 5071, 4869, 4677, 4492, 4316, 4148, 3987, 3833, 3686,                       // 40 - 49
  3545, 3415, 3291, 3172, 3058, 2948, 2844, 2743, 2646, 2554,                       // 50 - 59
  2465, 2379, 2297, 2219, 2143, 2070, 2001, 1933, 1869, 1807,                       // 60 - 69
  1747, 1690, 1634, 1581, 1530, 1481, 1433, 1388, 1344, 1301,                       // 70 - 79
  1261, 1221, 1183, 1147, 1111, 1077, 1045, 1013, 982, 953,                         // 80 - 89
  924, 897, 870, 845, 820, 796, 773, 751, 729, 708,                                 // 90 - 99
  688, 669, 650, 632, 615, 598, 581, 566, 550, 535,                                 // 100 - 109
  521, 507, 494, 481, 468, 456, 444, 433, 422, 411,                                 // 110 - 119
  400, 390, 381, 371, 362, 353, 344, 336, 328, 320,                                 // 120 - 129
  312, 304, 297, 290, 283, 277, 270, 264, 258, 252,                                 // 130 - 139
  246, 240, 235, 239, 224, 219, 215, 210, 205, 201, 196,                            // 140 - 150
};
const int coolant_ntc_resistance_count = sizeof(coolant_ntc_resistance_table) / sizeof(coolant_ntc_resistance_table[0]);

ADS7823Source::ADS7823Source(int index, uint8_t i2c_address, int bits, int mult, int div_) :
  AnalogSourceBase(index, 100, i2c_address, bits, mult, div_)
{
  if (index == INDEX_COOLANT_TEMP) {
    _thermistor = new NTCThermistor(25, 50, 10000, 3545, coolant_ntc_resistance_table, coolant_ntc_resistance_count, -50);
  }
};


ADS7823Source::~ADS7823Source(void)
{
  if (_thermistor) {
    delete _thermistor;
  }
}

void ADS7823Source::init(void) {
  if (_bits >= _min_bits && _bits <= _max_bits) {
    _valid = true;
  }

  if (!_valid) {
    Log.error("ADS7823@%0X/I2C is not configured correctly", _i2c_address);
    return;
  }

  Log.notice("Setting up ADS7823@%X/I2C", _i2c_address);
  // This ADC doesn't even have a control register.  Cool.
}

int32_t ADS7823Source::read_device(void) {
  if (!_valid) {
    return UNUSED_READING;
  }

  uint16_t raw_readings[4];
  i2c_read_data(0x00, (uint8_t *)&raw_readings, sizeof(raw_readings));

  int32_t accumulator = 0;
  for (int i = 0; i < 4; i++) {
    accumulator += raw_readings[i];
  }

  return accumulator / 4;
}

int32_t ADS7823Source::convert(int32_t reading) {
  if (_index == INDEX_COOLANT_TEMP) {
    int resistance = reading * 10000 / (4096 - reading);
    int retval = _thermistor->lookup(resistance);       // Takes ~ 10us, < 1% error
    // int retval2 = _thermistor->calculate(resistance);   // Takes ~ 650us
    return retval;
  } else {
    int retval = AnalogSourceBase::convert(reading);
    return retval;
  }
}

