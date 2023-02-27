#ifndef __eeprom_checksum_h_
#define __eeprom_checksum_h_

#include <Arduino.h>

uint8_t eeprom_checksum(uint8_t *buf, int len);

#endif
