#ifndef __wifi_h_
#define __wifi_h_

#include <WiFi.h>

#define WIFI_SSID       "default-ssid"
#define WIFI_SSID_LEN   12
#define WIFI_PSK        "default-psk"
#define WIFI_PSK_LEN    11
#define WIFI_SERVER     "192.168.12.243"
#define WIFI_SERVER_LEN 14
#define WIFI_PORT       "8192"
#define WIFI_PORT_LEN   4

#define CBOR_CURRENT_VERSION  1
enum {
  CBOR_VERSION,
  CBOR_PACKET_TYPE,
  CBOR_BUFFER,
  CBOR_FSM_STATE,
  CBOR_FSM_MODE,
  CBOR_BURN_POWER,
  CBOR_FLAME_DETECT,
  CBOR_COMBUSTION_FAN,
  CBOR_VEHICLE_FAN,
  CBOR_INTERNAL_TEMP,
  CBOR_OUTDOOR_TEMP,
  CBOR_COOLANT_TEMP,
  CBOR_EXHAUST_TEMP,
  CBOR_BATTERY_VOLT,
  CBOR_VSYS_VOLT,
  CBOR_GPIOS,
};

enum {
  CBOR_TYPE_KLINE,
  CBOR_TYPE_SENSORS,
};

enum {
  CBOR_GPIO_START_RUN,
  CBOR_GPIO_IGNITION,
  CBOR_GPIO_EMERG_STOP,
};

extern mutex_t wifi_mutex;
extern bool wifi_server_connected;
extern WiFiClient client;

void init_wifi(void);
void update_wifi(void);
void cbor_send(void);

#endif
