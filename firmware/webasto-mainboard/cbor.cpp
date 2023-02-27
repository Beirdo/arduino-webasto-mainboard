#include <Arduino.h>
#include <pico.h>
#include <CoreMutex.h>
#include <ArduinoLog.h>
#include <tinycbor.h>

#include "fsm.h"
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

  TinyCBOR.Encoder.create_map(6);

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

  TinyCBOR.Encoder.encode_int(CBOR_COMBUSTION_FAN);
  TinyCBOR.Encoder.encode_int(combustionFanPercent);

  TinyCBOR.Encoder.encode_int(CBOR_VEHICLE_FAN);
  TinyCBOR.Encoder.encode_int(vehicleFanPercent);

  TinyCBOR.Encoder.close_container();

  int len = TinyCBOR.Encoder.get_buffer_size();
 
  Log.notice("Queuing CBOR packet (len %d)", len);
  canbus_send(CANBUS_ID_MAINBOARD, cbor_tx_buf, len);
}
