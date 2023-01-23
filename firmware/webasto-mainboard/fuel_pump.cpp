#include <Arduino.h>
#include <pico.h>

#include "fuel_pump.h"


FuelPumpTimer fuelPumpTimer(&fuelPumpTimerCallback);

void fuelPumpTimerCallback(int timer_id, int delay_ms) {
  fuelPumpTimer.timerCallback(timer_id, delay_ms);
}

void FuelPumpTimer::abort(void) {
  setPeriod(0);  
}

void FuelPumpTimer::timerCallback(int timer_id, int delayed) {
  (void)timer_id;
  if (_next_level) {
    _period = _next_period;
    if (!_period) {
      digitalWrite(PIN_FUEL_PUMP, LOW);
      _enabled = false;
      return;
    }
    digitalWrite(PIN_FUEL_PUMP, HIGH);
    globalTimer.register_timer(TIMER_FUEL_PUMP, FUEL_PUMP_PULSE_LEN, _cb);
  } else {
    digitalWrite(PIN_FUEL_PUMP, LOW);
    globalTimer.register_timer(TIMER_FUEL_PUMP, _period - FUEL_PUMP_PULSE_LEN, _cb);
  }
  _next_level = !_next_level;
}

int FuelPumpTimer::getFuelPumpFrequency(void) {
  // returns mHz
  if (_next_period == 0) {
    return 0;
  }
  return ((2000000 / _next_period) + 1) / 2;
  
}

uint8_t FuelPumpTimer::getFuelPumpFrequencyKline(void) {
  return (((getFuelPumpFrequency() / 250) + 1) / 2) & 0xFF;
}

void FuelPumpTimer::setBurnPower(int watts) {
  if (watts == 0) {
    setPeriod(0);
  }

  int periodMs = clamp(DIESEL_VOL_COMB_ENERGY * FUEL_PUMP_DOSE_ML / watts, FUEL_PUMP_MIN_PERIOD, FUEL_PUMP_MAX_PERIOD);
  setPeriod(periodMs);
}

int FuelPumpTimer::getBurnPower(void) 
{
  // The density of diesel is between 820-845mg/ml.  We will use 832.5mg/ml for our calculations
  // The combustion energy of diesel is 44.80kJ/g
  // The volumetric combustion energy of diesel is then:
  // 44.80kJ/g * 0.8325g/ml = 37.296kJ/ml
  // The power of the heating is thus:
  // P = 37.296kJ/ml * pump dose [ml] / dose period [s]
  //   = (820.512 [kJ] / dose period [s]) [kJ/s]
  //   = (820512 [kJ] / dose period [ms]) [kJ/ms] or [J/s] or [W]

  if (_next_period == 0) {
    return 0;
  }
  return DIESEL_VOL_COMB_ENERGY * FUEL_PUMP_DOSE_ML / _next_period;
}

void FuelPumpTimer::setPeriod(int periodMs) {
  _next_period = periodMs;
  if (!_enabled) {
    _enabled = true;
    _next_level = true;
    timerCallback(0, 0);
  }
}
