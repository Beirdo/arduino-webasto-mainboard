#include <Arduino.h>
#include <pico.h>
#include <stdlib.h>
#include <I2C_eeprom.h>
#include <ArduinoLog.h>

#include "project.h"
#include "sensor_eeprom.h"

const char *capabilities_names[7] = {
  "external temperature",
  "battery voltage",
  "coolant temperature",
  "exhaust temperature",
  "ignition sense",
  "internal temperature",
  "flame detector",  
};

eeprom_data_t eeprom_data[8];
void *eeprom_devices[8];

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
  // TODO: still gonna be funky at rev 2
  memset(eeprom_data, 0xFF, sizeof(eeprom_data));

  int max_eeprom_data = sizeof(struct eeprom_v1_s);
  int min_eeprom_data = sizeof(struct eeprom_v1_s);

  // This will redefine the I2C bus used in analog.cpp, but the values are identical
  for (int i = 0; i < 8; i++) {
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

    Log.notice("Reading EEPROM on Sensor board %d", i);

    int len = eeprom->readBlock(0, buf, max_eeprom_data);
    if (len < min_eeprom_data) {
      Log.error("Sensor board EEPROM at index %d incomplete (%d bytes read)", i, len);
      delete eeprom;
      eeprom_devices[i] = 0;
      continue;
    }

    bool empty = true;
    for (int j = 0; j < len; j++) {
      if (buf[j] != 0xFF) {
        empty = false;
        break;
      }
    }

    if (empty) {
      Log.error("Sensor board EEPROM at index %d is empty", i, len);
      delete eeprom;
      eeprom_devices[i] = 0;
      continue;
    }

    // Check the checksum
    if (eeprom_checksum(buf, len)) {
      Log.error("Sensor board EEPROM at index %d has bad checksum", i, len);
      delete eeprom;
      eeprom_devices[i] = 0;
      continue;
    }

    uint8_t version = eeprom_data[i].version;
    bool too_small = false;
  
    switch (version) {
      case 1:
        if(len < sizeof(struct eeprom_v1_s)) {
          too_small = true;
        }
        break;
      default:
        Log.error("Sensor board EEPROM at index %s has an unsupported version (%d)", i, version);
        delete eeprom;
        eeprom_devices[i] = 0;
        continue;
    }

    if (too_small) {
      Log.error("Sensor board EEPROM at index %d incomplete for v%d (%d bytes read)", i, version, len);
      delete eeprom;
      eeprom_devices[1] = 0;
      continue;
    } 

    Log.notice("Found v%d Sensor Board at index %d with capabilities of 0x%02X", eeprom_data[i].version, i, eeprom_data[i].capabilities);
  }
}
