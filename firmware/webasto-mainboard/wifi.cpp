#include "cbor.h"
#include <Arduino.h>
#include <pico.h>
#include <CoreMutex.h>
#include <WiFi.h>
#include <cppQueue.h>
#include <ArduinoLog.h>
#include <tinycbor.h>

#include "global_timer.h"
#include "device_eeprom.h"
#include "wifi.h"
#include "fsm.h"
#include "analog.h"
#include "project.h"
#include "wbus.h"

#define WIFI_STRING_LEN 32

WiFiMulti multi;
uint8_t ssid[WIFI_STRING_LEN];
uint8_t psk[WIFI_STRING_LEN];
uint8_t *old_ssid = 0;
uint8_t *old_psk = 0;
mutex_t wifi_mutex;
bool wifi_connected = false;
bool wifi_server_connected = false;

WiFiClient client;
uint8_t server[128];
uint8_t port_str[16];
uint16_t port;

#define CBOR_BUF_SIZE   64
#define CBOR_BUF_COUNT  16

typedef uint8_t cbor_buf_t[CBOR_BUF_SIZE];

cbor_buf_t cbor_bufs[CBOR_BUF_COUNT];
cbor_buf_t cbor_rx_buf;
cbor_buf_t cbor_rx_wbus_buf;
int cbor_rx_tail;

mutex_t cbor_mutex;
int cbor_head = 0;
int cbor_tail = 0;

typedef struct {
  int index;
  int len;
} cbor_item_t;

cppQueue cbor_tx_q(sizeof(cbor_item_t), CBOR_BUF_COUNT, FIFO);


void wifi_startup_callback(int timer_id, int delay_ms);
void wifi_connection_callback(int timer_id, int delay_ms);


void wifi_startup_callback(int timer_id, int delay_ms)
{
  if (timer_id != TIMER_WIFI_STARTUP) {
    return;
  }

  (void)delay_ms;

  CoreMutex m(&wifi_mutex);

  get_device_info_string(13, ssid, 32);
  get_device_info_string(14, psk, 32);

  Log.notice("Connecting to WiFi SSID: %s", ssid);
  bool new_ap = false;
  if (!old_ssid || !old_psk) {
    new_ap = true;
  } else if (strcmp((const char *)ssid, (const char *)old_ssid)) {
    new_ap = true;
  } else if (strcmp((const char *)psk, (const char *)old_psk)) {
    new_ap = true;
  }

  if (new_ap) {
    old_ssid = ssid;
    old_psk = psk;

    multi.addAP((const char *)ssid, (const char *)psk);
  }

  wifi_connected = (multi.run() == WL_CONNECTED);
  wifi_server_connected = false;
  if (!wifi_connected) {
    Log.warning("Could not connect to WiFi, trying again in 10s");
    globalTimer.register_timer(TIMER_WIFI_STARTUP, 10000, wifi_startup_callback);
  } else {
    Log.info("WiFi connected");
    Log.info("IP Address: %s", WiFi.localIP());
    wifi_connection_callback(TIMER_WIFI_CONNECT, 0);
  }
}

void wifi_connection_callback(int timer_id, int delay_ms)
{
  if (timer_id != TIMER_WIFI_CONNECT) {
    return;
  }

  (void)delay_ms;

  CoreMutex m(&wifi_mutex);

  get_device_info_string(15, server, 128);
  get_device_info_string(16, port_str, 16);
  port = atoi((const char *)port_str);

  Log.notice("Connecting to server: %s:%d", server, port);

  wifi_server_connected = client.connect(server, port);
  if (!wifi_server_connected) {
    Log.warning("Could not connect to server, trying again in 5s");
    globalTimer.register_timer(TIMER_WIFI_CONNECT, 5000, wifi_connection_callback);
  } else {
    Log.info("Server connected");
    Log.info("Local IP Address: %s:%d", client.localIP(), client.localPort());
    Log.info("Remote IP Address: %s:%d", client.remoteIP(), client.remotePort());
    cbor_rx_tail = 0;
  }
}

void init_wifi(void)
{
  mutex_init(&wifi_mutex);
  mutex_init(&cbor_mutex);
  wifi_startup_callback(TIMER_WIFI_STARTUP, 0);
  TinyCBOR.init();
}

void update_wifi(void)
{
  CoreMutex m(&cbor_mutex);
  bool flush = false;

  cbor_item_t item;
  while (!cbor_tx_q.isEmpty()) {
    cbor_tx_q.pop(&item);

    if (!client.connected() || !WiFi.connected()) {
      flush = true;
      continue;
    }

    const uint8_t *buf = (const uint8_t *)&cbor_bufs[item.index];
    int len = item.len;

    hexdump(buf, len, 16);
    client.write((const uint8_t *)&cbor_bufs[item.index], item.len);

    cbor_head += 1;
    cbor_head %= CBOR_BUF_COUNT;

    if (cbor_head == cbor_tail) {
      cbor_head = 0;
      cbor_tail = 0;
    }
  }

  if (flush) {
    if (!WiFi.connected() && !globalTimer.get_remaining_time(TIMER_WIFI_STARTUP)) {
      wifi_connected = false;
      wifi_startup_callback(TIMER_WIFI_STARTUP, 0);
      return;
    }

    if (!client.connected() && !globalTimer.get_remaining_time(TIMER_WIFI_CONNECT)) {
      wifi_server_connected = false;
      wifi_connection_callback(TIMER_WIFI_CONNECT, 0);
      return;
    }
  }

  while (client.available()) {
    cbor_rx_buf[cbor_rx_tail++] = client.read();
  }

  hexdump(cbor_rx_buf, cbor_rx_tail, 16);
  TinyCBOR.Parser.init(cbor_rx_buf, cbor_rx_tail, 0);

  int size = 0;
  uint8_t index;

  while (!TinyCBOR.Parser.at_end_of_data()) {
    int ty = TinyCBOR.Parser.get_type();
    switch (ty) {
      case CborMapType:
        size = TinyCBOR.Parser.get_map_length();
        TinyCBOR.Parser.enter_container();
        break;

      // CborInvalidType(0xFF) is equivalent to a break byte, and is put at the
      // end of a container.
      case CborInvalidType:
        if (TinyCBOR.Parser.at_end_of_container()) {
          TinyCBOR.Parser.leave_container();
        } else {
          // This is a load of crap.  Flush it and abort.  Don't forget to wipe.
          goto bail;
        }
        break;

      // Our keys are all simple type as are some of our values
      case CborSimpleType:
        {
          bool isKey = TinyCBOR.Parser.is_key();
          uint8_t value = TinyCBOR.Parser.get_simple_type();
          if (isKey) {
            index = value;
          } else if (index == CBOR_VERSION && value != CBOR_CURRENT_VERSION) {
            Log.warning("Wrong CBOR packet version: %d", value);
            goto bail;
          } else if (index == CBOR_PACKET_TYPE && value != CBOR_TYPE_WBUS) {
            Log.warning("Wrong CBOR buffer type: %d", value);
            goto bail;
          }
        }
        break;

      case CborByteStringType:
        {
          int str_len = TinyCBOR.Parser.get_string_length();
          if (str_len > CBOR_BUF_SIZE) {
            Log.warning("Received WBUS packet is too long (%d)", str_len);
            goto bail;
          }
          str_len = TinyCBOR.Parser.copy_byte_string(cbor_rx_wbus_buf, CBOR_BUF_SIZE);
          receive_wbus_from_cbor(cbor_rx_wbus_buf, str_len);
        }
        break;

      case CborTextStringType:
        TinyCBOR.Parser.get_string_length();
        TinyCBOR.Parser.copy_text_string((char *)cbor_rx_wbus_buf, CBOR_BUF_SIZE);
        break;

      case CborTagType:
        TinyCBOR.Parser.get_tag();
        break;

      case CborIntegerType:
        if (TinyCBOR.Parser.is_unsigned_integer()) {
          TinyCBOR.Parser.get_uint64();
        } else if (TinyCBOR.Parser.is_negative_integer()) {
          TinyCBOR.Parser.get_int64();
        } else if (TinyCBOR.Parser.is_integer()) {
          TinyCBOR.Parser.get_int64();
        }
        break;

      case CborBooleanType:
        TinyCBOR.Parser.get_boolean();
        break;

      case CborFloatType:
        TinyCBOR.Parser.get_float();
        break;

      case CborDoubleType:
        TinyCBOR.Parser.get_double();
        break;

      case CborHalfFloatType:
        break;

      case CborNullType:
        TinyCBOR.Parser.get_null();
        break;

      case CborUndefinedType:
        TinyCBOR.Parser.skip_undefined();
        break;

      default:
        goto bail;
    }
  }

bail:
  cbor_rx_tail = 0;
  return;
}

void cbor_send(const uint8_t *wbus_buf, int wbus_len)
{
   CoreMutex m(&cbor_mutex);

  cbor_item_t item;
  if ((cbor_tail + 1) % CBOR_BUF_COUNT == cbor_head) {
    return;
  }

  item.index = cbor_tail++;
  cbor_tail %= CBOR_BUF_COUNT;
  item.len = 0;

  TinyCBOR.Encoder.init(cbor_bufs[item.index], CBOR_BUF_SIZE);

  if (wbus_buf) {
    TinyCBOR.Encoder.create_map(3);

    // key values are integers (well, enum)
    TinyCBOR.Encoder.encode_simple_value(CBOR_VERSION);
    TinyCBOR.Encoder.encode_simple_value(CBOR_CURRENT_VERSION);

    TinyCBOR.Encoder.encode_simple_value(CBOR_PACKET_TYPE);
    TinyCBOR.Encoder.encode_simple_value(CBOR_TYPE_WBUS);

    TinyCBOR.Encoder.encode_simple_value(CBOR_BUFFER);
    TinyCBOR.Encoder.encode_byte_string(wbus_buf, wbus_len);

    TinyCBOR.Encoder.close_container();
  } else {
    TinyCBOR.Encoder.create_map(15);

    // key values are integers (well, enum)
    TinyCBOR.Encoder.encode_simple_value(CBOR_VERSION);
    TinyCBOR.Encoder.encode_simple_value(CBOR_CURRENT_VERSION);

    TinyCBOR.Encoder.encode_simple_value(CBOR_PACKET_TYPE);
    TinyCBOR.Encoder.encode_simple_value(CBOR_TYPE_SENSORS);

    mutex_enter_blocking(&fsm_mutex);
    TinyCBOR.Encoder.encode_simple_value(CBOR_FSM_STATE);
    TinyCBOR.Encoder.encode_simple_value(fsm_state);

    TinyCBOR.Encoder.encode_simple_value(CBOR_FSM_MODE);
    TinyCBOR.Encoder.encode_simple_value(fsm_mode);
    mutex_exit(&fsm_mutex);

    TinyCBOR.Encoder.encode_simple_value(CBOR_BURN_POWER);
    TinyCBOR.Encoder.encode_uint(fuelPumpTimer.getBurnPower());

    TinyCBOR.Encoder.encode_simple_value(CBOR_FLAME_DETECT);
    TinyCBOR.Encoder.encode_uint(flameDetectorSensor->get_value());

    TinyCBOR.Encoder.encode_simple_value(CBOR_COMBUSTION_FAN);
    TinyCBOR.Encoder.encode_simple_value(combustionFanPercent);

    TinyCBOR.Encoder.encode_simple_value(CBOR_VEHICLE_FAN);
    TinyCBOR.Encoder.encode_simple_value(vehicleFanPercent);

    TinyCBOR.Encoder.encode_simple_value(CBOR_INTERNAL_TEMP);
    TinyCBOR.Encoder.encode_int(internalTempSensor->get_value());

    TinyCBOR.Encoder.encode_simple_value(CBOR_OUTDOOR_TEMP);
    TinyCBOR.Encoder.encode_int(externalTempSensor->get_value());

    TinyCBOR.Encoder.encode_simple_value(CBOR_COOLANT_TEMP);
    TinyCBOR.Encoder.encode_int(coolantTempSensor->get_value());

    TinyCBOR.Encoder.encode_simple_value(CBOR_EXHAUST_TEMP);
    TinyCBOR.Encoder.encode_int(exhaustTempSensor->get_value());

    TinyCBOR.Encoder.encode_simple_value(CBOR_BATTERY_VOLT);
    TinyCBOR.Encoder.encode_int(batteryVoltageSensor->get_value());

    TinyCBOR.Encoder.encode_simple_value(CBOR_VSYS_VOLT);
    TinyCBOR.Encoder.encode_int(vsysVoltageSensor->get_value());

    TinyCBOR.Encoder.encode_simple_value(CBOR_GPIOS);
    uint8_t gpios = 0;
    gpios |= startRunSensor->get_value() ? BIT(CBOR_GPIO_START_RUN) : 0;
    gpios |= ignitionSenseSensor->get_value() ? BIT(CBOR_GPIO_IGNITION) : 0;
    gpios |= emergencyStopSensor->get_value() ? BIT(CBOR_GPIO_EMERG_STOP) : 0;
    TinyCBOR.Encoder.encode_simple_value(gpios);

    TinyCBOR.Encoder.close_container();
  }

  item.len = TinyCBOR.Encoder.get_buffer_size();

  cbor_tx_q.push(&item);
}
