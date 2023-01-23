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

void fuelPumpTimerCallback(int timer_id, int delay_ms);

class FuelPumpTimer {
  public:
    FuelPumpTimer(timer_callback cb) {
      _cb = cb;
      pinMode(PIN_FUEL_PUMP, OUTPUT);
      _enabled = false;
      abort();
    }
    
    void abort(void);
    void timerCallback(int timer_id, int delayed);
    int getFuelPumpFrequency(void);
    uint8_t getFuelPumpFrequencyKline(void);
    void setBurnPower(int watts);
    int getBurnPower(void);

  protected:
    void setPeriod(int periodMs);

  private:
    bool _enabled;
    int _period;
    int _next_period;
    bool _next_level;
    timer_callback _cb;
};

extern FuelPumpTimer fuelPumpTimer;

#endif
