#include "WiFiMulti.h"
#include "wl_definitions.h"
#include "lwip/ip4_addr.h"
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

mutex_t wifi_mutex;
WiFiMulti *wifi = 0;
uint8_t ssid[WIFI_STRING_LEN];
uint8_t psk[WIFI_STRING_LEN];
bool wifi_connected = false;
bool wifi_server_connected = false;
int wifi_timeout = 0;
int wifi_status = 0;

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
void update_cbor_tx();
void update_cbor_rx();

void wifi_startup_callback(int timer_id, int delay_ms)
{
  if (timer_id != TIMER_WIFI_STARTUP) {
    return;
  }

  CoreMutex m(&wifi_mutex);

  wifi_timeout = clamp<int>(wifi_timeout - delay_ms, 0, 10000);
  wifi_status = WiFi.status();

  if (wifi_status != WL_CONNECTED && !wifi_timeout) {
    if (wifi) {
      delete wifi;
    }

    wifi_timeout = 10000;
    wifi = new WiFiMulti();
    
    get_device_info_string(12, ssid, 32);
    get_device_info_string(13, psk, 32);
    wifi->addAP((const char *)ssid, (const char *)psk);

    Log.notice("Connecting to WiFi SSID: %s", ssid);
  }

  wifi_timeout = clamp<int>(wifi_timeout - 200, 0, 10000);
  wifi_status = wifi->run(200);
  Log.notice("Status after run: %d", wifi_status);
  
  if (wifi_status != WL_CONNECTED) {
    globalTimer.register_timer(TIMER_WIFI_STARTUP, clamp<int>(wifi_timeout, 0, 500), wifi_startup_callback);
  } else {
    Log.notice("WiFi connected");
    IPAddress myIP = WiFi.localIP();    
    Log.notice("IP Address: %s", ip_ntoa(myIP));
    globalTimer.register_timer(TIMER_WIFI_CONNECT, 10, wifi_connection_callback);
  }
}

void wifi_connection_callback(int timer_id, int delay_ms)
{
  if (timer_id != TIMER_WIFI_CONNECT) {
    return;
  }

  CoreMutex m(&wifi_mutex);

  get_device_info_string(14, server, 128);
  get_device_info_string(15, port_str, 16);

  ip4_addr_t server_ip4;
  ip4addr_aton((const char *)server, &server_ip4);
  IPAddress server_ip(server_ip4);
  port = atoi((const char *)port_str);

  Log.notice("Connecting to server: %s:%d", ip_ntoa(server_ip), port);
  client.setTimeout(250);
  wifi_server_connected = client.connect(server_ip, port);

  if (!wifi_server_connected) {
    Log.warning("Could not connect to server, trying again in 5s");
    client.stop();
    globalTimer.register_timer(TIMER_WIFI_CONNECT, 5000, wifi_connection_callback);
  } else {
    Log.notice("Server connected");
    Log.notice("Local IP Address: %s:%d", ip_ntoa(client.localIP()), client.localPort());
    Log.notice("Remote IP Address: %s:%d", ip_ntoa(client.remoteIP()), client.remotePort());
    cbor_rx_tail = 0;
  }
}

void init_wifi(void)
{
  mutex_init(&wifi_mutex);
  mutex_init(&cbor_mutex);
  globalTimer.register_timer(TIMER_WIFI_STARTUP, 10, wifi_startup_callback);
  TinyCBOR.init();
}

void update_wifi(void)
{
  update_cbor_tx();
  update_cbor_rx();
}

void update_cbor_tx(void) 
{
  CoreMutex m(&cbor_mutex);
  bool flush = cbor_tx_q.isEmpty();

  cbor_item_t item;
  while (!cbor_tx_q.isEmpty()) {
    cbor_tx_q.pop(&item);

    cbor_head = item.index + 1;
    cbor_head %= CBOR_BUF_COUNT;

    if (!client.connected() || WiFi.status() != WL_CONNECTED) {
      Log.notice("Flushing TX queue.  Disconnected");
      flush = true;
      continue;
    }

    const uint8_t *buf = (const uint8_t *)&cbor_bufs[item.index];
    int len = item.len;

    hexdump(buf, len, 16);
    client.write((const char *)buf, len);
  }

  if (flush) {
    if (WiFi.status() != WL_CONNECTED && !wifi_timeout && !globalTimer.get_remaining_time(TIMER_WIFI_STARTUP)) {
      Log.warning("WiFi no longer connected!");
      globalTimer.register_timer(TIMER_WIFI_STARTUP, 10, wifi_startup_callback); 
      return;
    }

    if (WiFi.status() == WL_CONNECTED && !client.connected() && !globalTimer.get_remaining_time(TIMER_WIFI_CONNECT)) {
      Log.warning("Server no longer connected!");
      wifi_server_connected = false;
      client.stop();
      globalTimer.register_timer(TIMER_WIFI_CONNECT, 10, wifi_connection_callback);
      return;
    }
  }
}

void update_cbor_rx(void)
{
  while (client.available()) {
    cbor_rx_buf[cbor_rx_tail++] = client.read();
  }

  if (cbor_rx_tail == 0) {
    return;
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

      // Our keys are all integers as are most/all of our values
      case CborIntegerType:
        {
          bool isKey = TinyCBOR.Parser.is_key();
          int64_t value;
          
          if (isKey) {
            index = TinyCBOR.Parser.get_int();
            continue;
          } else if (TinyCBOR.Parser.is_unsigned_integer()) {
            value = (int64_t)TinyCBOR.Parser.get_uint64();
          } else if (TinyCBOR.Parser.is_negative_integer()) {
            value = TinyCBOR.Parser.get_int64();
          } else if (TinyCBOR.Parser.is_integer()) {
            value = TinyCBOR.Parser.get_int64();
          }

          if (index == CBOR_VERSION && value != CBOR_CURRENT_VERSION) {
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

      case CborSimpleType:
        TinyCBOR.Parser.get_simple_type();
        break;

      case CborTagType:
        TinyCBOR.Parser.get_tag();
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
    Log.notice("Head: %d, Tail: %d", cbor_head, cbor_tail);
    return;
  }

  item.index = cbor_tail++;
  cbor_tail %= CBOR_BUF_COUNT;
  item.len = 0;

  TinyCBOR.Encoder.init(cbor_bufs[item.index], CBOR_BUF_SIZE);

  if (wbus_buf) {
    TinyCBOR.Encoder.create_map(3);

    // key values are integers (well, enum)
    TinyCBOR.Encoder.encode_int(CBOR_VERSION);
    TinyCBOR.Encoder.encode_int(CBOR_CURRENT_VERSION);

    TinyCBOR.Encoder.encode_int(CBOR_PACKET_TYPE);
    TinyCBOR.Encoder.encode_int(CBOR_TYPE_WBUS);

    TinyCBOR.Encoder.encode_int(CBOR_BUFFER);
    TinyCBOR.Encoder.encode_byte_string(wbus_buf, wbus_len);

    TinyCBOR.Encoder.close_container();
  } else {
    TinyCBOR.Encoder.create_map(15);

    // key values are integers (well, enum)
    TinyCBOR.Encoder.encode_int(CBOR_VERSION);
    TinyCBOR.Encoder.encode_int(CBOR_CURRENT_VERSION);

    TinyCBOR.Encoder.encode_int(CBOR_PACKET_TYPE);
    TinyCBOR.Encoder.encode_int(CBOR_TYPE_SENSORS);

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
  }

  item.len = TinyCBOR.Encoder.get_buffer_size();

  cbor_tx_q.push(&item);
}
