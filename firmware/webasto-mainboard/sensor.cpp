#include <Arduino.h>
#include <sensor.h>
#include <canbus_ids.h>

#include "internal_adc.h"
#include "ina219.h"
#include "sensor_registry.h"
#include "fsm.h"

void init_sensors(void)
{
  // On the mainboard
  sensorRegistry.add(CANBUS_ID_INTERNAL_TEMP, new InternalADCSensor(CANBUS_ID_INTERNAL_TEMP, 4, 12));
  sensorRegistry.add(CANBUS_ID_FLAME_DETECTOR, new INA219Sensor(CANBUS_ID_FLAME_DETECTOR, 0x4F, 12, &glowPlugInEnable));
  sensorRegistry.add(CANBUS_ID_VSYS_VOLTAGE, new InternalADCSensor(CANBUS_ID_VSYS_VOLTAGE, 2, 12));

  // Remote CANBus
  sensorRegistry.add(CANBUS_ID_EXTERNAL_TEMP, new RemoteCANBusSensor(CANBUS_ID_EXTERNAL_TEMP, 2, 100));
  sensorRegistry.add(CANBUS_ID_BATTERY_VOLTAGE, new RemoteCANBusSensor(CANBUS_ID_BATTERY_VOLTAGE, 2, 100));
  sensorRegistry.add(CANBUS_ID_COOLANT_TEMP_WEBASTO, new RemoteCANBusSensor(CANBUS_ID_COOLANT_TEMP_WEBASTO, 2, 100));
  sensorRegistry.add(CANBUS_ID_EXHAUST_TEMP, new RemoteCANBusSensor(CANBUS_ID_EXHAUST_TEMP, 2, 100));
  sensorRegistry.add(CANBUS_ID_IGNITION_SENSE, new RemoteCANBusSensor(CANBUS_ID_IGNITION_SENSE, 1, 1));
  sensorRegistry.add(CANBUS_ID_EMERGENCY_STOP, new RemoteCANBusSensor(CANBUS_ID_EMERGENCY_STOP, 1, 1));
  sensorRegistry.add(CANBUS_ID_START_RUN, new RemoteCANBusSensor(CANBUS_ID_START_RUN, 1, 1));

  // Remote LINBus
  sensorRegistry.add(CANBUS_ID_VEHICLE_FAN_PERCENT, new RemoteLINBusSensor(CANBUS_ID_VEHICLE_FAN_PERCENT, 1, 1, 2));
  sensorRegistry.add(CANBUS_ID_VEHICLE_FAN_SPEED, new RemoteLINBusSensor(CANBUS_ID_VEHICLE_FAN_SPEED, 2, 50));
  sensorRegistry.add(CANBUS_ID_VEHICLE_FAN_INT_TEMP, new RemoteLINBusSensor(CANBUS_ID_VEHICLE_FAN_INT_TEMP, 1, 1));
  sensorRegistry.add(CANBUS_ID_VEHICLE_FAN_EXT_TEMP, new RemoteLINBusSensor(CANBUS_ID_VEHICLE_FAN_EXT_TEMP, 2, 100));
}

void update_sensors(void)
{

}


void RemoteCANBusSensor::do_feedback(void)
{
  switch (_id) {
    case CANBUS_ID_EXTERNAL_TEMP:
      {
        OutdoorTempEvent event;
        event.value = (int)_value;
        WebastoControlFSM::dispatch(event);
      }
      break;
    case CANBUS_ID_BATTERY_VOLTAGE:
      {
        BatteryLevelEvent event;
        event.value = _value;
        WebastoControlFSM::dispatch(event);
      }
      break;
    case CANBUS_ID_COOLANT_TEMP_WEBASTO:
      {
        CoolantTempEvent event;
        event.value = (int)_value;
        WebastoControlFSM::dispatch(event);
      }
      break;
    case CANBUS_ID_EXHAUST_TEMP:
      {
        ExhaustTempEvent event;
        event.value = (int)_value;
        WebastoControlFSM::dispatch(event);
      }
      break;
    case CANBUS_ID_IGNITION_SENSE:
      {
        IgnitionEvent event;
        event.enable = (bool)_value;
        WebastoControlFSM::dispatch(event);
      }
      break;
    case CANBUS_ID_START_RUN:
      {
        StartRunEvent event;
        event.enable = (bool)_value;
        WebastoControlFSM::dispatch(event);
      }
      break;
    case CANBUS_ID_EMERGENCY_STOP:
      {
        EmergencyStopEvent event;
        event.enable = (bool)_value;
        WebastoControlFSM::dispatch(event);
      }
      break;
    default:
      break;
   }
}

void RemoteLINBusSensor::do_feedback(void)
{
  switch (_id) {
    case CANBUS_ID_VEHICLE_FAN_SPEED:
    default:
      break;
  }
}
