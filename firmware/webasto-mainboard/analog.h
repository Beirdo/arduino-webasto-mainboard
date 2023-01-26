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

extern AnalogSourceBase *externalTempSensor;
extern AnalogSourceBase *batteryVoltageSensor;
extern AnalogSourceBase *coolantTempSensor;
extern AnalogSourceBase *exhaustTempSensor;
extern AnalogSourceBase *ignitionSenseSensor;
extern InternalADCSource *internalTempSensor;
extern INA219Source *flameDetectorSensor;
extern InternalGPIODigitalSource *startRunSensor;
extern InternalGPIODigitalSource *emergencyStopSensor;

#endif
