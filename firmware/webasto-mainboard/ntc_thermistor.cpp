#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "ntc_thermistor.h"
#include "analog_source.h"
#include "project.h"


NTCThermistor::NTCThermistor(int t0, int t1, int r0, int r1, const int *table, int count, int offset)
{
  _table = table;
  _count = count;
  _offset = offset;

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
