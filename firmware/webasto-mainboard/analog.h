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

extern DS2482Source *externalTempSensor;
extern ADS7823Source *batteryVoltageSensor;
extern MCP96L01Source *coolantTempSensor;
extern MCP96L01Source *exhaustTempSensor;
extern PCA9501DigitalSource *ignitionSenseSensor;
extern InternalADCSource *internalTempSensor;
extern INA219Source *flameDetectorSensor;
extern InternalGPIODigitalSource *startRunSensor;

#endif
