#include <Arduino.h>
#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>

#include "project.h"
#include "analog.h"
#include "analog_source.h"
#include "ds2482.h"
#include "ads7823.h"
#include "mcp96l01.h"
#include "internal_adc.h"
#include "ina219.h"
#include "pca9501.h"
#include "sensor_eeprom.h"
#include "internal_gpio.h"
#include "fsm.h"
#include "dummy.h"

#define OFFBOARD_SENSOR_COUNT 5
#define ONBOARD_SENSOR_COUNT  4

AnalogSourceBase *sensors[OFFBOARD_SENSOR_COUNT + ONBOARD_SENSOR_COUNT];

AnalogSourceBase *externalTempSensor;
AnalogSourceBase *batteryVoltageSensor;
AnalogSourceBase *coolantTempSensor;
AnalogSourceBase *exhaustTempSensor;
AnalogSourceBase *ignitionSenseSensor;

InternalADCSource *internalTempSensor;
INA219Source *flameDetectorSensor;
InternalGPIODigitalSource *startRunSensor;
InternalGPIODigitalSource *emergencyStopSensor;


void init_analog(void)
{
  Wire.begin();

  for (int i = 0; i < 5; i++) {
    sensors[i] = new DummySource(i);

    for (int j = 0; j < 8; j++) {
      if (!eeprom_devices[j]) {
        continue;
      }

      uint8_t cap = eeprom_data[j].current.capabilities & (1 << i);
      if (!cap) {
        continue;
      }

      Log.notice("Found %s sensor on sensor board %d", capabilities_names[i], j);      

      switch(i) {
        case 0:
          sensors[i] = new DS2482Source(i, eeprom_data[j].current.addr_ds2482, 9);
          break;
        case 1:
          sensors[i] = new ADS7823Source(i, eeprom_data[j].current.addr_ads7823, 12, 19767, 4096);
          break;
        case 2:
        case 3:
          sensors[i] = new MCP96L01Source(i, eeprom_data[j].current.addr_mcp96l01, 16, TYPE_K, 4);
          break;
        case 4:
          sensors[i] = new PCA9501DigitalSource(i, eeprom_data[j].current.addr_pca9501_gpio, PCA9501_VINN);
          break;
        default:
          break;
      }      
    }
  }

  externalTempSensor = sensors[0];
  batteryVoltageSensor = sensors[1];
  coolantTempSensor = sensors[2];
  exhaustTempSensor = sensors[3];
  ignitionSenseSensor = sensors[4];

  internalTempSensor = new InternalADCSource(5, 4, 12);
  flameDetectorSensor = new INA219Source(6, 0x4F, 12, &glowPlugInEnable);
  startRunSensor = new InternalGPIODigitalSource(7, PIN_START_RUN);
  emergencyStopSensor = new InternalGPIODigitalSource(8, PIN_EMERGENCY_STOP);

  sensors[5] = internalTempSensor;
  sensors[6] = flameDetectorSensor;  
  sensors[7] = startRunSensor;
  sensors[8] = emergencyStopSensor;

  // Now init() the suckers
  for (int i = 0; i < OFFBOARD_SENSOR_COUNT + ONBOARD_SENSOR_COUNT; i++) {
    Log.notice("Initializing %s sensor", capabilities_names[i]);
    sensors[i]->init();
  }
}

void update_analog(void) 
{
  // Now update() the suckers
  for (int i = 0; i < OFFBOARD_SENSOR_COUNT + ONBOARD_SENSOR_COUNT; i++) {
#ifdef VERBOSE_LOGGING
    Log.notice("Updating %s sensor", capabilities_names[i]);
#endif
    sensors[i]->update();
#ifdef VERBOSE_LOGGING
    Log.notice("Feeding back %s sensor to FSM", capabilities_names[i]);
#endif
    sensors[i]->feedback(i);
  }
}
