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

#define OFFBOARD_SENSOR_COUNT 6
#define ONBOARD_SENSOR_COUNT  4

AnalogSourceBase *sensors[OFFBOARD_SENSOR_COUNT + ONBOARD_SENSOR_COUNT];

AnalogSourceBase *externalTempSensor;
AnalogSourceBase *batteryVoltageSensor;
AnalogSourceBase *coolantTempSensor;
AnalogSourceBase *exhaustTempSensor;
AnalogSourceBase *ignitionSenseSensor;
AnalogSourceBase *emergencyStopSensor;

InternalADCSource *internalTempSensor;
INA219Source *flameDetectorSensor;
InternalGPIODigitalSource *startRunSensor;
InternalADCSource *vsysVoltageSensor;


void init_analog(void)
{
  Wire.begin();

  for (int i = 0; i < OFFBOARD_SENSOR_COUNT; i++) {
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
        case INDEX_EXTERNAL_TEMP:
          sensors[i] = new DS2482Source(i, eeprom_data[j].current.addr_ds2482, 9);
          break;
        case INDEX_BATTERY_VOLTAGE:
        case INDEX_COOLANT_TEMP:
          sensors[i] = new ADS7823Source(i, eeprom_data[j].current.addr_ads7823, 12, 19767, 4096);
          break;
        case INDEX_EXHAUST_TEMP:
          sensors[i] = new MCP96L01Source(i, eeprom_data[j].current.addr_mcp96l01, 16, TYPE_K, 4);
          break;
        case INDEX_IGNITION_SENSE:
          sensors[i] = new PCA9501DigitalSource(i, eeprom_data[j].current.addr_pca9501_gpio, PCA9501_VINN);
          break;
        case INDEX_EMERGENCY_STOP:
          sensors[i] = new PCA9501DigitalSource(i, eeprom_data[i].current.addr_pca9501_gpio, PCA9501_VINN);
          break;  
        default:
          break;
      }      
    }
  }

  externalTempSensor = sensors[INDEX_EXTERNAL_TEMP];
  batteryVoltageSensor = sensors[INDEX_BATTERY_VOLTAGE];
  coolantTempSensor = sensors[INDEX_COOLANT_TEMP];
  exhaustTempSensor = sensors[INDEX_EXHAUST_TEMP];
  ignitionSenseSensor = sensors[INDEX_IGNITION_SENSE];
  emergencyStopSensor = sensors[INDEX_EMERGENCY_STOP];

  internalTempSensor = new InternalADCSource(INDEX_INTERNAL_TEMP, 4, 12);
  flameDetectorSensor = new INA219Source(INDEX_FLAME_DETECTOR, 0x4F, 12, &glowPlugInEnable);
  startRunSensor = new InternalGPIODigitalSource(INDEX_START_RUN, PIN_START_RUN);
  vsysVoltageSensor = new InternalADCSource(INDEX_VSYS_VOLTAGE, 3, 12);

  sensors[INDEX_INTERNAL_TEMP] = internalTempSensor;
  sensors[INDEX_FLAME_DETECTOR] = flameDetectorSensor;  
  sensors[INDEX_START_RUN] = startRunSensor;
  sensors[INDEX_VSYS_VOLTAGE] = vsysVoltageSensor;

  // Now init() the suckers
  for (int i = 0; i < OFFBOARD_SENSOR_COUNT + ONBOARD_SENSOR_COUNT; i++) {
    Log.notice("Initializing %s sensor", capabilities_names[i]);
    if (!sensors[i]) {
      Log.error("Missing class for %s sensor", capabilities_names[i]);
    }
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
    if (!sensors[i]) {
      continue;
    }
    sensors[i]->update();
#ifdef VERBOSE_LOGGING
    Log.notice("Feeding back %s sensor to FSM", capabilities_names[i]);
#endif
    sensors[i]->feedback(i);
  }
}
