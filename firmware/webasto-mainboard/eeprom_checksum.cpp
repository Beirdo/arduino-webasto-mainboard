#include <Arduino.h>

#include "eeprom_checksum.h"

uint8_t eeprom_checksum(uint8_t *buf, int len)
{
  uint checksum = 0;
  for (int i = 0; i < len; i++) {
    checksum ^= *buf;
  }
  return checksum;
}

