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
#include "pca9501.h"
#include "sensor_eeprom.h"
#include "internal_gpio.h"
#include "fsm.h"
#include "dummy.h"
#include "linbus_registers.h"
#include "canbus.h"

#define OFFBOARD_SENSOR_COUNT (INDEX_START_RUN + 1)
#define ONBOARD_SENSOR_COUNT  (INDEX_VSYS_VOLTAGE - INDEX_START_RUN)
#define TOTAL_SENSOR_COUNT    (INDEX_VSYS_VOLTAGE + 1)

AnalogSourceBase *sensors[TOTAL_SENSOR_COUNT];

AnalogSourceBase *externalTempSensor;
AnalogSourceBase *batteryVoltageSensor;
AnalogSourceBase *coolantTempSensor;
AnalogSourceBase *exhaustTempSensor;
AnalogSourceBase *ignitionSenseSensor;
AnalogSourceBase *emergencyStopSensor;
AnalogSourceBase *startRunSensor;

LINBusRegister *linbus_sensors[32];
uint8_t addr_linbus_bridge = 0xFF;
uint32_t linbus_slaves = 0;

void init_analog(void)
{
  Wire.begin();

  addr_linbus_bridge = 0xFF;

  for (int i = 0; i < 32; i++ ) {
    linbus_sensors[i] = 0;
  }

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
        case INDEX_START_RUN:
          sensors[i] = new PCA9501DigitalSource(i, eeprom_data[j].current.addr_pca9501_gpio, PCA9501_VINN);
          break;
        case INDEX_EMERGENCY_STOP:
          sensors[i] = new PCA9501DigitalSource(i, eeprom_data[i].current.addr_pca9501_gpio, PCA9501_VINN, true);
          break;
        case INDEX_LINBUS_BRIDGE:
          addr_linbus_bridge = eeprom_data[i].current.addr_linbus_bridge;
          linbus_slaves = eeprom_data[i].current.linbus_slaves;
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
  startRunSensor = sensors[INDEX_START_RUN];

  // Now init() the suckers
  for (int i = 0; i < TOTAL_SENSOR_COUNT; i++) {
    Log.notice("Initializing %s sensor", capabilities_names[i]);
    if (!sensors[i]) {
      Log.error("Missing class for %s sensor", capabilities_names[i]);
    }
    sensors[i]->init();
  }

  if (addr_linbus_bridge != 0xFF) {
    // Let's get LINBus stuff setup now
    for (uint8_t i = 0; i < 32; i++) {
      if (linbus_slaves & BIT(i)) {
        LINBusRegister tempreg(i, 0);
        uint16_t value = tempreg.get16u(0);  // Get board type and location
        uint8_t board_type = HI_BYTE(value);
        uint8_t location = LO_BYTE(value);

        switch (board_type) {
          case BOARD_TYPE_VALVE_CONTROL:
            linbus_sensors[i] = new LINBusRegisterValveControl(i, location);
            break;
          case BOARD_TYPE_COOLANT_TEMP:
            linbus_sensors[i] = new LINBusRegisterCoolantTemperature(i, location);
            break;
          case BOARD_TYPE_FLOW_SENSOR:
            linbus_sensors[i] = new LINBusRegisterFlowSensor(i, location);
            break;
          case BOARD_TYPE_FAN_CONTROL:
            linbus_sensors[i] = new LINBusRegisterFanControl(i, location);
            break;
          case BOARD_TYPE_PUMP_CONTROL:
            linbus_sensors[i] = new LINBusRegisterPumpControl(i, location);
            break;
          case BOARD_TYPE_PELTIER_CONTROL:
            linbus_sensors[i] = new LINBusRegisterPeltierControl(i, location);
            break;
          default:
            break;
        }
      }
    }

    // Initialize each that we found.
    for (int i = 0; i < 32; i++) {
      if (linbus_sensors[i]) {
        linbus_sensors[i]->init();
      }      
    }

  }    
}

void update_analog(void)
{
  // Now update() the suckers
  for (int i = 0; i < TOTAL_SENSOR_COUNT; i++) {
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
    sensors[i]->feedback();
  }

  for (int i = 0; i < 32; i++) {
    if (linbus_sensors[i]) {
      linbus_sensors[i]->update();
      linbus_sensors[i]->feedback();
    }    
  }
}

void send_analog_to_canbus(int id)
{
#if 0
  int32_t value;
  switch (id) {
    case CANBUS_ID_INTERNAL_TEMP:
      value = internalTempSensor->get_value();
      break;
      
    case CANBUS_ID_FLAME_DETECTOR:
      value = flameDetectorSensor->get_value();
      break;

    case CANBUS_ID_VSYS_VOLTAGE:
      value = vsysVoltageSensor->get_value();
    default:
      return;
  }

  canbus_output_value<int32_t>(id, value);
#endif
}

void receive_analog_from_canbus(int id, uint8_t *buf, int len)
{
  AnalogSourceBase *sensor = 0;
  uint32_t raw = 0;

  buf += len;
  for (int i = 0; i < len && i < 4; i++) {
    raw <<= 8;
    raw |= *(--buf);    
  }

  int32_t value = (int32_t)raw;

  switch (id) {
    case CANBUS_ID_EXTERNAL_TEMP:
      sensor = externalTempSensor;
      break;
      
    case CANBUS_ID_BATTERY_VOLTAGE:
      sensor = batteryVoltageSensor;
      break;

    case CANBUS_ID_COOLANT_TEMP_WEBASTO:
      sensor = coolantTempSensor;
      break;

    case CANBUS_ID_EXHAUST_TEMP:
      sensor = exhaustTempSensor;
      break;

    case CANBUS_ID_IGNITION_SENSE:
      sensor = ignitionSenseSensor;
      break;

    case CANBUS_ID_EMERGENCY_STOP:
      sensor = emergencyStopSensor;
      break;

    case CANBUS_ID_START_RUN:
      sensor = startRunSensor;
      break;

    default:
      break;
  }

  if (!sensor) {
    return;
  }

  sensor->set_value(value);
  sensor->feedback();
}
