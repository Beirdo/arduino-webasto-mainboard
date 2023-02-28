#include <Arduino.h>

#include "ntc_thermistor.h"
#include "project.h"


const long int coolant_ntc_resistance_table[] = {
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


NTCThermistor thermistor(25, 50, 10000, 3545, coolant_ntc_resistance_table, coolant_ntc_resistance_count, -50);


NTCThermistor::NTCThermistor(int t0, int t1, int r0, int r1, const long int *table, int count, int offset)
{
  _table = table;
  _count = count;
  _offset = offset;

#ifdef FLOATING_POINT_MATH
  _r0 = (double)r0;
  _t0 = (double)t0 + 273.15;

  double r = (double)r1;
  double t = (double)t1 + 273.15;

  // using the beta equation:
  // 1/T = 1/T0 + (1/beta) ln(R/R0)
  //
  // Solving for beta with T = 50C (323.15K), T0 = 25C (298.15K), R = 3545, R0 = 10000 (table above)
  // (T0 - T) / (T * T0) = (1/beta) ln(R/R0)
  // beta = T * T0 * ln(R/R0) / (T0 - T)

  _beta = t * _t0 * log(r / _r0) / (_t0 - t);
#endif
}

int32_t NTCThermistor::lookup(int rth)
{
  // This takes about 10us on the RP2040, and is less than 1% error from the calculation below
  //
  // look up the resistor value in the table (indexed by degC, with an offset)
  // Find which two readings we fall between and treat it as a piecewise linear mapping
  // between the closest two readings.
  if (!_table || _count < 2) {
    return UNUSED_READING;
  }

  rth = clamp<int>(rth, _table[_count - 1], _table[0]);

  if (rth == _table[0]) {
    return _offset * 100;
  }

  if (rth == _table[_count - 1]) {
    return (_offset + _count) * 100;
  }

  for (int i = 0; i < _count - 1; i++) {
    if (_table[i] >= rth && _table[i + 1] <= rth) {
      return map<int>(rth, _table[i], _table[i + 1], i * 100, (i + 1) * 100) + (_offset * 100);
    }
  }

  return UNUSED_READING;
}

#ifdef FLOATING_POINT_MATH
int32_t NTCThermistor::calculate(int rth)
{
  // This takes about 650us on the RP2040
  //
  // using the beta equation:
  // 1/T = 1/T0 + (1/beta) ln(R/R0)
  //
  // Solving now for T, then converting to deg C, and then to 1/100C:
  double rhs = (1.0 / _t0) + (1.0 / _beta) * log((double)rth / _r0);
  return (int32_t)(((1.0 / rhs) - 273.15) * 100.0);
}
#endif