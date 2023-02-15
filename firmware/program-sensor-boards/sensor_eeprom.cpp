#include <Arduino.h>
#include <pico.h>
#include <stdlib.h>
#include <I2C_eeprom.h>
#include <ArduinoLog.h>

#include "project.h"
#include "sensor_eeprom.h"

eeprom_data_t eeprom_data[4] = {
  {1, 0, 0, 0x13, 0x30, 0xFF, 0x48, 0x18, 0xFF, 0x00000000},  // External temperature, battery voltage, ignition sense
  {1, 0, 1, 0x24, 0x31, 0xFF, 0x49, 0xFF, 0xFF, 0x00000000},  // Coolant temperature, emergency stop
  {1, 0, 2, 0x08, 0x32, 0x60, 0xFF, 0xFF, 0xFF, 0x00000000},  // Exhaust temperature
  {1, 0, 3, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0x7B. 0x00000000},  // LINBus Bridge
};
void *eeprom_devices[4];

uint8_t eeprom_checksum(uint8_t *buf, int len)
{
  uint checksum = 0;
  for (int i = 0; i < len; i++) {
    checksum ^= *buf;
  }
  return checksum;
}

void init_eeprom(void) 
{
  // This will redefine the I2C bus used in analog.cpp, but the values are identical
  for (int i = 0; i < 4; i++) {
    // scan I2C bus for the EEPROM, one per sensor board
    I2C_eeprom *eeprom = new I2C_eeprom(PCA9501_I2C_EEPROM_ADDR | (i & 0x0F), PCA9501_DEVICE_SIZE, &Wire);
    if (!eeprom->begin()) {
      Log.warning("No sensor board EEPROM found at index %d", i);
      delete eeprom;
      eeprom_devices[i] = 0;
      continue;
    }

    eeprom_devices[i] = eeprom;

    uint8_t *buf = (uint8_t *)&eeprom_data[i];

    Log.notice("Writing EEPROM on Sensor board %d", i);

    int max_len = sizeof(eeprom_data[i]);
    eeprom_data[i].checksum = 0;
    eeprom_data[i].checksum = eeprom_checksum((uint8_t *)&eeprom_data[i], max_len);
    int len = eeprom->writeBlock(0, buf, max_len);
    if (len < max_len) {
      Log.error("Sensor board EEPROM at index %d incomplete (%d bytes writte)", i, len);
      delete eeprom;
      eeprom_devices[i] = 0;
      continue;
    }
  }
}

 
