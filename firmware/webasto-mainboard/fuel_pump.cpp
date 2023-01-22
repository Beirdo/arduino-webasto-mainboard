#include <Arduino.h>
#include <pico.h>

#include "fuel_pump.h"


FuelPumpTimer fuelPumpTimer(&fuelPumpTimerCallback);

void fuelPumpTimerCallback(int delay_ms) {
  fuelPumpTimer.timerCallback(delay_ms);
}
