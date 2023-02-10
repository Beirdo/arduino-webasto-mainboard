#ifndef __analog_h
#define __analog_h

#include <pico.h>
#include "ds2482.h"
#include "ads7823.h"
#include "mcp96l01.h"
#include "internal_adc.h"
#include "ina219.h"
#include "pca9501.h"
#include "internal_gpio.h"
#include "fsm.h"

void init_analog(void);
void update_analog(void);

enum {
  INDEX_EXTERNAL_TEMP,
  INDEX_BATTERY_VOLTAGE,
  INDEX_COOLANT_TEMP,
  INDEX_EXHAUST_TEMP,
  INDEX_IGNITION_SENSE,
  INDEX_EMERGENCY_STOP,
  INDEX_INTERNAL_TEMP,
  INDEX_FLAME_DETECTOR,
  INDEX_START_RUN,
  INDEX_VSYS_VOLTAGE,
};

extern AnalogSourceBase *externalTempSensor;
extern AnalogSourceBase *batteryVoltageSensor;
extern AnalogSourceBase *coolantTempSensor;
extern AnalogSourceBase *exhaustTempSensor;
extern AnalogSourceBase *ignitionSenseSensor;
extern AnalogSourceBase *emergencyStopSensor;
extern InternalADCSource *internalTempSensor;
extern INA219Source *flameDetectorSensor;
extern InternalGPIODigitalSource *startRunSensor;
extern InternalADCSource *vsysVoltageSensor;

#endif
