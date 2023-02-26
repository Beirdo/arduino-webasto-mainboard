#ifndef __analog_h
#define __analog_h

#include <pico.h>
#include "ds2482.h"
#include "ads7823.h"
#include "mcp96l01.h"
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
  INDEX_START_RUN,
  INDEX_INTERNAL_TEMP,
  INDEX_FLAME_DETECTOR,
  INDEX_VSYS_VOLTAGE,
  INDEX_LINBUS_BRIDGE,
  MAX_INDEX,
};

extern AnalogSourceBase *externalTempSensor;
extern AnalogSourceBase *batteryVoltageSensor;
extern AnalogSourceBase *coolantTempSensor;
extern AnalogSourceBase *exhaustTempSensor;
extern AnalogSourceBase *ignitionSenseSensor;
extern AnalogSourceBase *emergencyStopSensor;
extern AnalogSourceBase *startRunSensor;

void send_analog_to_canbus(int id);
void receive_analog_from_canbus(int id, uint8_t *buf, int len);

#endif
