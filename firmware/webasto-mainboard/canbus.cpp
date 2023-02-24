#include "pins_arduino.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include <ACAN2517FD.h>
#include <pico.h>
#include <SPI.h>
#include <stdlib.h>

#include "project.h"
#include "canbus.h"

ACAN2517FD can(PIN_SPI0_SS, SPI, PIN_CAN_INT);

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
    Log.error("CAN Initialization error code: 0x08X", errorCode);
    _initialized = false;
  } else {
    Log.notice("Initialized CANBus on MCP2517");
    _initialized = true;
  }

  return _initialized;
}

int CANBus::write(const char *buf, int len)
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
  msg.id = 0x0000;
  msg.ext = false;

  if (!can.tryToSend(msg)) {
    return 0;
  }
  return len;
}

int CANBUS::read(const char *buf, int len)
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

  memcpy(buf, msg.data, msg.len);
  return msg.len;
}

bool CANBUS::available(void)
{
  if (!_initialized) {
    return false;
  }

  return can.available();
}