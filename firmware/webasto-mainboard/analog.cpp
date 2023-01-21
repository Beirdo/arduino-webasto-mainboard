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

#define OFFBOARD_SENSOR_COUNT 5
#define ONBOARD_SENSOR_COUNT  2

AnalogSourceBase *sensors[OFFBOARD_SENSOR_COUNT + ONBOARD_SENSOR_COUNT];

DS2482Source *externalTempSensor;
ADS7823Source *batteryVoltageSensor;
MCP96L01Source *coolantTempSensor;
MCP96L01Source *exhaustTempSensor;
PCA9501DigitalSource *ignitionSenseSensor;

InternalADCSource *internalTempSensor;
INA219Source *flameDetectorSensor;

// should move to glow_plug later.
volatile bool glow_plug_in_enable;
volatile bool glow_plug_out_enable;


void set_open_drain_pin(int pinNum, int value)
{
  if (value) {
    pinMode(pinNum, INPUT);
    digitalWrite(pinNum, LOW);  // shutoff builtin pullup
  } else {
    pinMode(pinNum, OUTPUT);
    digitalWrite(pinNum, LOW);
  }
}

void init_analog(void)
{
  Wire.begin();

  glow_plug_in_enable = false;
  set_open_drain_pin(GLOW_PLUG_IN_EN_PIN, glow_plug_in_enable);

  glow_plug_out_enable = false;
  set_open_drain_pin(GLOW_PLUG_OUT_EN_PIN, glow_plug_out_enable);

  for (int i = 0; i < 5; i++) {
    sensors[i] = 0;

    for (int j = 0; j < 8; j++) {
      if (!eeprom_devices[j]) {
        continue;
      }

      uint8_t cap = eeprom_data[j].capabilities & (1 << i);
      if (!cap) {
        continue;
      }

      Log.notice("Found %s sensor on sensor board %d", capabilities_names[i], j);      

      switch(i) {
        case 0:
          sensors[i] = new DS2482Source(eeprom_data[j].addr_ds2482, 9);
          break;
        case 1:
          sensors[i] = new ADS7823Source(eeprom_data[j].addr_ads7823, 12, 19767, 4096);
          break;
        case 2:
        case 3:
          sensors[i] = new MCP96L01Source(eeprom_data[j].addr_mcp96l01, 16, TYPE_K, 4);
          break;
        case 4:
          sensors[i] = new PCA9501DigitalSource(eeprom_data[j].addr_pca9501_gpio, PCA9501_VINN);
          break;
        default:
          break;
      }      
    }
  }

  externalTempSensor = static_cast<DS2482Source *>(sensors[0]);
  batteryVoltageSensor = static_cast<ADS7823Source *>(sensors[1]);
  coolantTempSensor = static_cast<MCP96L01Source *>(sensors[2]);
  exhaustTempSensor = static_cast<MCP96L01Source *>(sensors[3]);
  ignitionSenseSensor = static_cast<PCA9501DigitalSource *>(sensors[4]);

  internalTempSensor = new InternalADCSource(4, 12);
  flameDetectorSensor = new INA219Source(0x4F, 12, &glow_plug_in_enable);
  
  sensors[ONBOARD_SENSOR_COUNT] = internalTempSensor;
  sensors[ONBOARD_SENSOR_COUNT + 1] = flameDetectorSensor;  

  // Now init() the suckers
  for (int i = 0; i < OFFBOARD_SENSOR_COUNT + ONBOARD_SENSOR_COUNT; i++) {
    if (!sensors[i]) {
      Log.warning("No sensor found for type: %s", capabilities_names[i]);
      continue;
    }
    sensors[i]->init();
  }
}

void update_analog(void) 
{
  // Now init() the suckers
  for (int i = 0; i < OFFBOARD_SENSOR_COUNT + ONBOARD_SENSOR_COUNT; i++) {
    if (!sensors[i]) {
      Log.warning("No sensor found for type: %s", capabilities_names[i]);
      continue;
    }
    sensors[i]->update();
  }
}
