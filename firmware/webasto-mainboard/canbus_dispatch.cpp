#include <Arduino.h>
#include <ArduinoLog.h>
#include <pico.h>

#include "canbus.h"
#include "canbus_ids.h"
#include "canbus_dispatch.h"
#include "wbus.h"
#include "cbor.h"
#include "sensor_registry.h"


void canbus_dispatch(int id, uint8_t *buf, int len, uint8_t type)
{
  id &= ~(CANBUS_ID_WRITE_MODIFIER);
  switch (id) {
    case CANBUS_ID_MAINBOARD:
      // Someone is requesting the CBOR
      if (type == CAN_REMOTE) {
        cbor_send();
      }
      break;

    case CANBUS_ID_WBUS:
      // This is an incoming WBUS packet, tunneled in from K-Line
      receive_wbus_from_canbus(buf, len);
      break;

    case CANBUS_ID_INTERNAL_TEMP:
    case CANBUS_ID_FLAME_DETECTOR:
    case CANBUS_ID_VSYS_VOLTAGE:
      if (type == CAN_REMOTE) {
        Sensor *sensor = sensorRegistry.get(id);
        if (sensor) {
          sensor->update();
        }
      }
      break;
    case CANBUS_ID_EXTERNAL_TEMP:
    case CANBUS_ID_BATTERY_VOLTAGE:
    case CANBUS_ID_COOLANT_TEMP_WEBASTO:
    case CANBUS_ID_EXHAUST_TEMP:
    case CANBUS_ID_IGNITION_SENSE:
    case CANBUS_ID_EMERGENCY_STOP:
    case CANBUS_ID_START_RUN:
      {
        RemoteCANBusSensor *sensor = dynamic_cast<RemoteCANBusSensor *>(sensorRegistry.get(id));
        if (sensor) {
          int32_t value = sensor->convert_from_packet(buf, len);
          sensor->set_value(value);
        }
      }
      break;

    default:
      break;
  }
}
