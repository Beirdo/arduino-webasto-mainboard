#include <Arduino.h>
#include <ArduinoLog.h>
#include <ACAN2517FD.h>
#include <pico.h>
#include <SPI.h>
#include <stdlib.h>
#include <cppQueue.h>

#include "project.h"
#include "canbus.h"
#include "wbus.h"

ACAN2517FD can(PIN_SPI0_SS, SPI, PIN_CAN_INT);
CANBus canbus;

#define CANBUS_BUF_COUNT  16

typedef struct {
  int id;
  uint8_t buf[CANBUS_BUF_SIZE];
  int len;
} canbus_buf_t;

canbus_buf_t canbus_tx_bufs[CANBUS_BUF_COUNT];
uint8_t canbus_rx_buf[CANBUS_BUF_SIZE];

mutex_t canbus_mutex;
int canbus_head = 0;
int canbus_tail = 0;

cppQueue canbus_tx_q(sizeof(int), CANBUS_BUF_COUNT, FIFO);

void canbus_dispatch(int id, uint8_t *buf, int len);

bool CANBus::begin(void)
{
  SPI.setRX(PIN_SPI0_MISO);
  SPI.setTX(PIN_SPI0_MOSI);
  SPI.setSCK(PIN_SPI0_SCK);
  SPI.setCS(PIN_SPI0_SS);

  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_20MHz,
                               1000 * 1000, DataBitRateFactor::x8) ;
  const uint32_t errorCode = can.begin(settings, [] { can.isr () ; }) ;  
  if (errorCode) {
    Log.error("CAN Initialization error code: %X", errorCode);
    _initialized = false;
  } else {
    Log.notice("Initialized CANBus on MCP2517");
    _initialized = true;
  }

  return _initialized;
}

int CANBus::write(int id, const char *buf, int len)
{
  if (!_initialized) {
    return 0;    
  }

  CANFDMessage msg;
  if (len > 64) {
    return 0;
  }

  memcpy(msg.data, buf, len);
  msg.len = len;
  msg.pad();
  msg.id = id;
  msg.ext = false;

  if (!can.tryToSend(msg)) {
    return 0;
  }
  return len;
}

int CANBus::read(int *id, const char *buf, int len)
{
  if (!_initialized) {
    return 0;
  }

  CANFDMessage msg;
  if (!can.receive(msg)) {
    return 0;
  }

  if (len < msg.len) {
    return 0;
  }

  memcpy((char *)buf, msg.data, msg.len);

  if (id) {
    *id = msg.id;
  }

  return msg.len;
}

bool CANBus::available(void)
{
  if (!_initialized) {
    return false;
  }

  return can.available();
}

void init_canbus(void)
{
  mutex_init(&canbus_mutex);
  canbus.begin();  
}

void update_canbus_tx(void) 
{
  while (!canbus_tx_q.isEmpty()) {
    CoreMutex m(&canbus_mutex);

    int index;
    canbus_tx_q.pop(&index);

    canbus_head = (index + 1) % CANBUS_BUF_COUNT;

    canbus_buf_t *item = &canbus_tx_bufs[index];
    const uint8_t *buf = (const uint8_t *)item->buf;
    int len = item->len;
    int id = item->id;

    if (!len) {
      continue;
    }

    int retlen = canbus.write(id, (const char *)buf, len);
    Log.notice("CANBus ID %X: Sent %d bytes of %d", retlen, len);

#ifdef HEXDUMP_TX
    hexdump(buf, len, 16);
#endif    
  }
}

void update_canbus_rx(void)
{
  while (canbus.available()) {
    int id;
    int len = canbus.read(&id, (const char *)canbus_rx_buf, CANBUS_BUF_SIZE);

    if (!len) {
      continue;
    }

    Log.notice("CANBus ID %X: Received %d bytes", id, len);

#ifdef HEXDUMP_TX
    hexdump(canbus_rx_buf, len, 16);
#endif

    canbus_dispatch(id, canbus_rx_buf, len);
  }
}

void canbus_dispatch(int id, uint8_t *buf, int len)
{
  switch(id) {
    case CANBUS_ID_MAINBOARD:
      // Why are we recieving stuff addressed to this?  Ignore.
      break;

    case CANBUS_ID_WBUS:
      // This is an incoming WBUS packet, tunneled in from K-Line
      receive_wbus_from_canbus(buf, len);
      break;

    default:
      break;
  }
}

void canbus_send(int id, uint8_t *buf, int len)
{
  CoreMutex m(&canbus_mutex);
  
  if ((canbus_tail + 1) % CANBUS_BUF_COUNT == canbus_head) {
    // We are out of space.  Discard the oldest
    canbus_head = (canbus_head + 1) % CANBUS_BUF_COUNT;
  }
  
  int index = canbus_tail;
  canbus_tail = (canbus_tail + 1) % CANBUS_BUF_COUNT;

  canbus_buf_t *item = &canbus_tx_bufs[index];
  item->id = id;
  item->len = len;
  memcpy(item->buf, buf, len);

  canbus_tx_q.push(&index);
}