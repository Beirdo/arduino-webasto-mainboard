#ifndef __kline_packet_h
#define __kline_packet_h

#include <pico.h>

typedef struct {
  uint8_t *buf;
  int len;
} klinePacket_t;


#endif