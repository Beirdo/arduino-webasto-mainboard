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

  cbor_item_t item;
  while (!cbor_tx_q.isEmpty()) {
    cbor_tx_q.pop(&item);

    if (client.connected()) {
      client.write((const uint8_t *)&cbor_bufs[item.index], item.len);
    }
    
    cbor_head += 1;
    cbor_head %= CBOR_BUF_COUNT;

    if (cbor_head == cbor_tail) {
      cbor_head = 0;
      cbor_tail = 0;
    }     
  }

  while (client.available()) {
    // for now, just eat the characters.  Will tie this into KLine.
    client.read();
  }
}

void cbor_send(const uint8_t *kline_buf, int kline_len) 
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

  if (kline_buf) {
    TinyCBOR.Encoder.create_map(3);

    // key values are integers (well, enum)
    TinyCBOR.Encoder.encode_simple_value(CBOR_VERSION);
    TinyCBOR.Encoder.encode_simple_value(CBOR_CURRENT_VERSION);

    TinyCBOR.Encoder.encode_simple_value(CBOR_PACKET_TYPE);
    TinyCBOR.Encoder.encode_simple_value(CBOR_TYPE_KLINE);

    TinyCBOR.Encoder.encode_simple_value(CBOR_BUFFER);
    TinyCBOR.Encoder.encode_byte_string(kline_buf, kline_len);

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