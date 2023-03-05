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

#define MIN_FUEL_NEED           (double)(0.6)     // At idle - about 1300W
#define MAX_FUEL_NEED_BURNING   (double)(2.464)   // MAX rated power of 5200W
#define MAX_FUEL_NEED_PRIMING   (double)(3.5)     // for priming only. - about 7200W!

#define FUEL_PUMP_PULSE_LEN     9       // ms high for the pulse
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
    void setFuelNeed(double need);

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
