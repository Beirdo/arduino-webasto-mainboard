#include <Arduino.h>
#include <canbus_ids.h>
#include <canbus_dispatch.h>

#include "project.h"

void canbus_dispatch(int id, uint8_t *buf, int len, uint8_t type)
{
  switch (id) {
    case CANBUS_ID_COOLANT_TEMP_WEBASTO:
      coolantTempSensor.update();
      break;

    case CANBUS_ID_BATTERY_VOLTAGE:
      batteryVoltageSensor.update();
      break;

    case CANBUS_ID_COOLANT_TEMP_WEBASTO | CANBUS_ID_WRITE_MODIFIER:
    case CANBUS_ID_BATTERY_VOLTAGE | CANBUS_ID_WRITE_MODIFIER:
    default:
      // Neither of these sensors have any control knobs.  Ignore em.
      break;
  }
}