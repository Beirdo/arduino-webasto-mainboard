#include <Arduino.h>
#include <sensor.h>
#include <canbus_ids.h>

#include "internal_adc.h"
#include "ina219.h"
#include "sensor_registry.h"
#include "fsm.h"

void sensor_init(void)
{
  sensorRegistry.add(CANBUS_ID_INTERNAL_TEMP, new InternalADCSensor(CANBUS_ID_INTERNAL_TEMP, 4, 12));
  sensorRegistry.add(CANBUS_ID_FLAME_DETECTOR, new INA219Sensor(CANBUS_ID_FLAME_DETECTOR, 0x4F, 12, &glowPlugInEnable));
  sensorRegistry.add(CANBUS_ID_VSYS_VOLTAGE, new InternalADCSensor(CANBUS_ID_VSYS_VOLTAGE, 2, 12));

}

void sensor_update(void)
{

}
