#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "ads7823.h"
#include "analog.h"

int32_t ntc_lookup_temperature_by_resistance(int resistance);
int32_t ntc_calc_temperature_by_resistance(int resistance);


void ADS7823Source::init(void) {
  if (_bits >= _min_bits && _bits <= _max_bits) {
    _valid = true;
  }

  if (!_valid) {
    Log.error("ADS7823@0x%02X/I2C is not configured correctly", _i2c_address);
    return;
  }

  Log.notice("Setting up ADS7823@0x%02X/I2C", _i2c_address);
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
    unsigned long t1 = micros();
    int retval = ntc_lookup_temperature_by_resistance(resistance);
    unsigned long t2 = micros();
    int retval2 = ntc_calc_temperature_by_resistance(resistance);
    unsigned long t3 = micros();
    Log.notice("Lookup: %d in %dus, Calculate: %d in %dus", retval, t2 - t1, retval2, t3 - t2);

    float error = ((double)retval - (double)retval2) / (double)retval2 * 100.0;
    Log.notice("Error: %f%%", error);
    return retval2;
  } else {
    int retval = AnalogSourceBase::convert(reading);
    return retval;
  }
}

const int ntc_temperature_offset = -50;
const int ntc_resistance_table[] = {
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
const int ntc_resistance_count = sizeof(ntc_resistance_table) / sizeof(ntc_resistance_table[0]);

int32_t ntc_lookup_temperature_by_resistance(int resistance)
{
  resistance = clamp(resistance, ntc_resistance_table[ntc_resistance_count - 1], ntc_resistance_table[0]);

  if (resistance == ntc_resistance_table[0]) {
    return ntc_temperature_offset * 100;
  }

  if (resistance == ntc_resistance_table[ntc_resistance_count - 1]) {
    return (ntc_temperature_offset + ntc_resistance_count) * 100;
  }

  for (int i = 0; i < ntc_resistance_count - 1; i++) {
    if (ntc_resistance_table[i] >= resistance && ntc_resistance_table[i + 1] <= resistance) {
      return map(resistance, ntc_resistance_table[i], ntc_resistance_table[i + 1], i * 100, (i + 1) * 100) + 
        (ntc_temperature_offset * 100);
    }
  }
  return UNUSED_READING;
}

int32_t ntc_calc_temperature_by_resistance(int resistance)
{
  // using the beta equation:
  // 1/T = 1/T0 + (1/beta) ln(R/R0)
  //
  // Solving for beta with T = 50C (323.15K), T0 = 25C (298.15K), R = 3545, R0 = 10000 (table above)
  // (T0 - T) / (T * T0) = (1/beta) ln(R/R0)
  // beta = T * T0 * ln(R/R0) / (T0 - T)
  //      = 323.15 * 298.15 * ln(3545 / 10000) / (298.15 - 323.15)
  //      = 96347.1725 * -1.0370459 / (-25.0)
  //      = 3996.6615
  //
  // Solving now for T, then converting to deg C, and then to 1/100C:
  double rhs = (1.0 / 298.15) * (1.0 / 3996.6615) * log((double)resistance / 10000.0);
  return (int32_t)(((1.0 / rhs) - 273.15) * 100.0);
}