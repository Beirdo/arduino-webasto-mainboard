#ifndef __analog_h
#define __analog_h

#include <pico.h>
#include "ds2482.h"
#include "ads7823.h"
#include "mcp96l01.h"
#include "internal_adc.h"
#include "ina219.h"

void init_analog(void);
void update_analog(void);

extern volatile bool glow_plug_in_enable;
extern volatile bool glow_plug_out_enable;
void set_open_drain_pin(int pinNum, int value);


extern DS2482Source externalTempSensor;
extern ADS7823Source batteryVoltageSensor;
extern MCP96L01Source coolantTempSensor;
extern MCP96L01Source exhaustTempSensor;
extern InternalADCSource internalTempSensor;
extern INA219Source flameDetectorSensor;

#endif
