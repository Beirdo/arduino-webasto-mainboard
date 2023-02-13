#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>
#include <EEPROM.h>
#include <stdlib.h>
#include <string.h>

#include "device_eeprom.h"
#include "sensor_eeprom.h"
#include "project.h"
#include "wifi.h"

#define EEPROM_SIZE 4096

device_info_t device_info[DEVICE_INFO_COUNT];
device_index_t device_length;

const uint8_t default_device_info_00[] = {0x09, 0x00, 0x62, 0x07, 0x43};               // Device ID Number
const uint8_t default_device_info_01[] = {50, 19,                                      // Hardware version (Week/Year)
                                          3, 50, 19, 5, 61,                            // Software version (day of week, week, year, major, minor)
                                          3, 50, 19, 5, 61};                           // Software EEPROM version (day of week, week, year, major, minor)
const uint8_t default_device_info_02[] = {0x09, 0x00, 0x74, 0x76, 0x45, 0x00};         // Data Set ID Number
const uint8_t default_device_info_03[] = {1, 23, 23};                                  // Control Unit Manufacture date (Day Month Year each one byte)
const uint8_t default_device_info_04[] = {12, 25, 19};                                 // Heater manufacture date (Day, month, year. Each one byte)
const uint8_t default_device_info_05[] = {0};                                          // 1 byte, no idea
const uint8_t default_device_info_06[] = "1K0815071E 0707";                            // Customer ID Number (i.e. VW Part number)
const uint8_t default_device_info_07[] = {0x4C, 0x17, 0x29, 0x32, 0x53, 0x26};         // Serial Number
const uint8_t default_device_info_08[] = {0x33};                                       // W-BUS version. 1 byte, each nibble is one digit. 0x33 means version 3.3
const uint8_t default_device_info_09[] = "PQ48 SH";                                    // Device Name: ASCII Text string.
const uint8_t default_device_info_10[] = { 0x78, 0x7C, 0x00, 0x0A, 0x50, 0x20, 0x00};  // W-BUS code. Flags of supported subsystems
const uint8_t default_device_info_11[] = { 00, 00, 00, 00, 0x1E };                     // Software ID


const device_info_t default_device_info[DEVICE_INFO_COUNT] = {
  { (uint8_t *)default_device_info_00, 5 },
  { (uint8_t *)default_device_info_01, 12 },
  { (uint8_t *)default_device_info_02, 6 },
  { (uint8_t *)default_device_info_03, 3 },
  { (uint8_t *)default_device_info_04, 3 },
  { (uint8_t *)default_device_info_05, 1 },
  { (uint8_t *)default_device_info_06, 15 },
  { (uint8_t *)default_device_info_07, 6 },
  { (uint8_t *)default_device_info_08, 1 },
  { (uint8_t *)default_device_info_09, 7 },
  { (uint8_t *)default_device_info_10, 7 },
  { (uint8_t *)default_device_info_11, 5 },
  { (uint8_t *)WIFI_SSID, WIFI_SSID_LEN },
  { (uint8_t *)WIFI_PSK, WIFI_PSK_LEN },
  { (uint8_t *)WIFI_SERVER, WIFI_SERVER_LEN },
  { (uint8_t *)WIFI_PORT, WIFI_PORT_LEN },
};

bool device_info_dirty;
bool device_info_valid;

void init_device_eeprom(void)
{
  uint8_t checksum;

  Log.notice("Reading device info from onboard EEPROM");
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, checksum);
  EEPROM.get(1, device_length);

  bool empty = false;
  for (int i = 0; i < DEVICE_INFO_COUNT; i++) {
    if (device_length[i] == 0xFFFF) {
      empty = true;
      break;
    }
  }

  uint8_t calc_checksum = checksum;

  if (!empty) {
    device_info_valid = true;

    int addr = sizeof(device_length) + 1;
    for (int i = 0; i < DEVICE_INFO_COUNT && addr < EEPROM_SIZE; i++) {
      int len = device_length[i];
      device_info[i].len = len;
      calc_checksum ^= HI_BYTE(len);
      calc_checksum ^= LO_BYTE(len);
      device_info[i].buf = (uint8_t *)malloc(len);
      for (int j = 0; j < len && addr < EEPROM_SIZE; j++) {
        uint8_t ch = EEPROM.read(addr++);
        calc_checksum ^= ch;
        device_info[i].buf[j] = ch;
      }
    }

    if (addr == EEPROM_SIZE) {
      Log.error("EEPROM data > %d bytes, resetting to default", EEPROM_SIZE);
      device_info_valid = false;
    }

    if (calc_checksum) {
      Log.error("EEPROM data has a bad checksum, resetting to default");
    }
  }

  if (empty || !device_info_valid || calc_checksum) {
    for (int i = 0; i < DEVICE_INFO_COUNT; i++) {
      int len = default_device_info[i].len;
      device_length[i] = len;
      device_info[i].len = len;
      device_info[i].buf = (uint8_t *)malloc(len);
      memcpy(device_info[i].buf, default_device_info[i].buf, len);
    }
    device_info_dirty = true;
    device_info_valid = true;
  }

  EEPROM.end();
}

void update_device_eeprom(void)
{
  if (device_info_dirty && device_info_valid) {
    Log.notice("Writing back dirty cache to internal EEPROM");
    uint8_t checksum = 0x00;

    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(1, device_length);
    int addr = sizeof(device_length) + 1;
    for (int i = 0; i < DEVICE_INFO_COUNT; i++) {
      int len = device_length[i];
      checksum ^= HI_BYTE(len);
      checksum ^= LO_BYTE(len);
      for (int j = 0; j < len; j++) {
        uint8_t ch = device_info[i].buf[j];
        checksum ^= ch;
        EEPROM.write(addr++, ch);
      }
    }

    EEPROM.write(0, checksum);

    for (int i = addr; i < EEPROM_SIZE; i++) {
      EEPROM.write(addr++, 0xFF);
    }

    if (addr == EEPROM_SIZE) {
      Log.error("EEPROM data > %d bytes!", EEPROM_SIZE);
      device_info_valid = false;
    }

    EEPROM.commit();
    EEPROM.end();
  }
}

device_info_t *get_device_info(int index)
{
  if (!device_info_valid || index < 0 || index >= DEVICE_INFO_COUNT) {
    return 0;
  }

  return &(device_info[index]);
}

int get_device_info_string(int index, uint8_t *buf, int len)
{
  device_info_t *dev = get_device_info(index);

  if (!dev || !buf) {
    return 0;
  }

  len = clamp<int>(len - 1, 0, dev->len);
  if (!len) {
    return 0;
  }

  memcpy(buf, dev->buf, len);
  buf[len] = 0;

  return len;
}
