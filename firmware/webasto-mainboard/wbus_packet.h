#ifndef __wbus_packet_h
#define __wbus_packet_h

#include <pico.h>

typedef struct {
  uint8_t *buf;
  int len;
  bool fromWiFi;
} wbusPacket_t;


#endif
