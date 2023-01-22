#ifndef __fuel_pump_h
#define __fuel_pump_h

#include <Arduino.h>
#include <pico.h>

#include "project.h"
#include "global_timer.h"

#define FUEL_PUMP_PULSE_LEN   30
#define FUEL_PUMP_MIN_PERIOD  (2 * FUEL_PUMP_PULSE_LEN)
#define FUEL_PUMP_MAX_PERIOD  500


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
  
    void setPeriod(int periodMs) {
      _next_period = periodMs;
      if (!_enabled) {
        _enabled = true;
        _next_level = true;
        timerCallback(0);
      }
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

  private:
    bool _enabled;
    int _period;
    int _next_period;
    bool _next_level;
    timer_callback _cb;
};

extern FuelPumpTimer fuelPumpTimer;

#endif
