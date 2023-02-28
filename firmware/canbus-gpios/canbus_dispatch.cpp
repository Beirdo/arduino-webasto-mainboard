#include <Arduino.h>
#include <canbus_ids.h>
#include <canbus_dispatch.h>

#include "project.h"

void canbus_dispatch(int id, uint8_t *buf, int len, uint8_t type)
{
  switch (id) {
    case CANBUS_ID_START_RUN:
      startRunSensor.update();
      break;

    case CANBUS_ID_IGNITION_SENSE:
      ignitionSenseSensor.update();
      break;

    case CANBUS_ID_EMERGENCY_STOP:
      emergencyStopSensor.update();
      break;

    case CANBUS_ID_START_RUN | CANBUS_ID_WRITE_MODIFIER:
    case CANBUS_ID_IGNITION_SENSE | CANBUS_ID_WRITE_MODIFIER:
    case CANBUS_ID_EMERGENCY_STOP | CANBUS_ID_WRITE_MODIFIER:
    default:
      // None of these sensors have any control knobs.  Ignore em.
      break;
  }
}