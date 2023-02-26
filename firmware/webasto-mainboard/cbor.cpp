#include <Arduino.h>
#include <pico.h>
#include <CoreMutex.h>
#include <ArduinoLog.h>
#include <tinycbor.h>

#include "fsm.h"
#include "analog.h"
#include "project.h"
#include "cbor.h"
#include "canbus.h"

mutex_t cbor_mutex;
uint8_t cbor_tx_buf[CANBUS_BUF_SIZE];

void init_cbor(void)
{
  mutex_init(&cbor_mutex);
  TinyCBOR.init();
}

void cbor_send(void)
{
  // Make the CBOR encoder thread-safe
  CoreMutex m(&cbor_mutex);

  TinyCBOR.Encoder.init(cbor_tx_buf, CANBUS_BUF_SIZE);

  TinyCBOR.Encoder.create_map(14);

  // key values are integers (well, enum)
  TinyCBOR.Encoder.encode_int(CBOR_VERSION);
  TinyCBOR.Encoder.encode_int(CBOR_CURRENT_VERSION);

  mutex_enter_blocking(&fsm_mutex);
  TinyCBOR.Encoder.encode_int(CBOR_FSM_STATE);
  TinyCBOR.Encoder.encode_int((uint8_t)(fsm_state & 0xFF));

  TinyCBOR.Encoder.encode_int(CBOR_FSM_MODE);
  TinyCBOR.Encoder.encode_int((uint8_t)(fsm_mode & 0xFF));
  mutex_exit(&fsm_mutex);

  TinyCBOR.Encoder.encode_int(CBOR_BURN_POWER);
  TinyCBOR.Encoder.encode_uint(fuelPumpTimer.getBurnPower());

  TinyCBOR.Encoder.encode_int(CBOR_FLAME_DETECT);
  TinyCBOR.Encoder.encode_uint(flameDetectorSensor->get_value());

  TinyCBOR.Encoder.encode_int(CBOR_COMBUSTION_FAN);
  TinyCBOR.Encoder.encode_int(combustionFanPercent);

  TinyCBOR.Encoder.encode_int(CBOR_VEHICLE_FAN);
  TinyCBOR.Encoder.encode_int(vehicleFanPercent);

  TinyCBOR.Encoder.encode_int(CBOR_INTERNAL_TEMP);
  TinyCBOR.Encoder.encode_int(internalTempSensor->get_value());

  TinyCBOR.Encoder.encode_int(CBOR_OUTDOOR_TEMP);
  TinyCBOR.Encoder.encode_int(externalTempSensor->get_value());

  TinyCBOR.Encoder.encode_int(CBOR_COOLANT_TEMP);
  TinyCBOR.Encoder.encode_int(coolantTempSensor->get_value());

  TinyCBOR.Encoder.encode_int(CBOR_EXHAUST_TEMP);
  TinyCBOR.Encoder.encode_int(exhaustTempSensor->get_value());

  TinyCBOR.Encoder.encode_int(CBOR_BATTERY_VOLT);
  TinyCBOR.Encoder.encode_int(batteryVoltageSensor->get_value());

  TinyCBOR.Encoder.encode_int(CBOR_VSYS_VOLT);
  TinyCBOR.Encoder.encode_int(vsysVoltageSensor->get_value());

  TinyCBOR.Encoder.encode_int(CBOR_GPIOS);
  uint8_t gpios = 0;
  gpios |= startRunSensor->get_value() ? BIT(CBOR_GPIO_START_RUN) : 0;
  gpios |= ignitionSenseSensor->get_value() ? BIT(CBOR_GPIO_IGNITION) : 0;
  gpios |= emergencyStopSensor->get_value() ? BIT(CBOR_GPIO_EMERG_STOP) : 0;
  TinyCBOR.Encoder.encode_int((uint8_t)(gpios & 0xFF));

  TinyCBOR.Encoder.close_container();

  int len = TinyCBOR.Encoder.get_buffer_size();
 
  Log.notice("Queuing CBOR packet (len %d)", len);
  canbus_send(CANBUS_ID_MAINBOARD, cbor_tx_buf, len);
}
