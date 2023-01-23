#ifndef __fuel_pump_h
#define __fuel_pump_h

#include <Arduino.h>
#include <pico.h>
#include <assert.h>

#include "project.h"
#include "global_timer.h"

#define DIESEL_VOL_COMB_ENERGY  37296   // kJ/ml
#define MIN_POWER               500     // we will adjust this for a proper value
#define MAX_RATED_POWER         5200    // Stated rating for my Thermatop C
#define FUEL_PUMP_DOSE_ML       22

#define FUEL_PUMP_PULSE_LEN     30      // ms high for the pulse
#define FUEL_PUMP_MIN_PERIOD  (DIESEL_VOL_COMB_ENERGY * FUEL_PUMP_DOSE_ML / MAX_RATED_POWER)
#define FUEL_PUMP_MAX_PERIOD  (DIESEL_VOL_COMB_ENERGY * FUEL_PUMP_DOSE_ML / MIN_POWER)

static_assert(FUEL_PUMP_MIN_PERIOD >= 2 * FUEL_PUMP_PULSE_LEN, "Min period requires > 50% duty cycle.");

void fuelPumpTimerCallback(int delay_ms);

class FuelPumpTimer {
  public:
    FuelPumpTimer(timer_callback cb) {
      _cb = cb;
      pinMode(PIN_FUEL_PUMP, OUTPUT);
      _enabled = false;
      abort();
    }
    
    void abort(void) {
      setPeriod(0);  
    }
  
    void timerCallback(int delayed) {
      if (_next_level) {
        _period = _next_period;
        if (!_period) {
          digitalWrite(PIN_FUEL_PUMP, LOW);
          _enabled = false;
          return;
        }
        digitalWrite(PIN_FUEL_PUMP, HIGH);
        globalTimer.register_timer(FUEL_PUMP_PULSE_LEN, _cb);
      } else {
        digitalWrite(PIN_FUEL_PUMP, LOW);
        globalTimer.register_timer(_period - FUEL_PUMP_PULSE_LEN, _cb);
      }
      _next_level = !_next_level;
    }

    int getFuelPumpFrequency(void) {
      // returns mHz
      if (_next_period == 0) {
        return 0;
      }
      return ((2000000 / _next_period) + 1) / 2;
      
    }

    uint8_t getFuelPumpFrequencyKline(void) {
      return (((getFuelPumpFrequency() / 250) + 1) / 2) & 0xFF;
    }

    void setBurnPower(int watts) {
      if (watts == 0) {
        setPeriod(0);
      }

      int periodMs = clamp(DIESEL_VOL_COMB_ENERGY * FUEL_PUMP_DOSE_ML / watts, FUEL_PUMP_MIN_PERIOD, FUEL_PUMP_MAX_PERIOD);
      setPeriod(periodMs);
    }

    int getBurnPower(void) 
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

  protected:
    void setPeriod(int periodMs) {
      _next_period = periodMs;
      if (!_enabled) {
        _enabled = true;
        _next_level = true;
        timerCallback(0);
      }
    }

  private:
    bool _enabled;
    int _period;
    int _next_period;
    bool _next_level;
    timer_callback _cb;
};

extern FuelPumpTimer fuelPumpTimer;

#endif
