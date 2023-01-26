#include <Arduino.h>
#include <pico.h>
#include <stdlib.h>
#include <string.h>
#include <I2C_eeprom.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>

#include "project.h"
#include "sensor_eeprom.h"
#include "fram.h"
#include "analog.h"
#include "fsm.h"
#include "beeper.h"

fram_data_t fram_data;
bool fram_dirty = false;
I2C_eeprom *fram;
mutex_t fram_mutex;

int fram_lengths[] = {
  sizeof(struct fram_v1_s),
};

void initalize_fram_data(uint8_t version, uint8_t *buf)
{
  if (version > MAX_FRAM_VERSION || version < 1) {
    return;
  }

  switch(version) {
    case 1:
      {
        memset(buf, 0x00, sizeof(fram_data));
        struct fram_v1_s *data = (struct fram_v1_s *)buf;
        data->version = 1;
        data->checksum = eeprom_checksum(buf, fram_lengths[0]);
        fram_dirty = true;
      }
      break;
    defaults:
      break;
  }
}

void init_fram(void) 
{
  mutex_init(&fram_mutex);
  memset(&fram_data, 0xFF, sizeof(fram_data));

  int max_fram_data = -1;
  int min_fram_data = CY15E004J_DEVICE_SIZE + 1;

  for (int i = 0; i < MAX_FRAM_VERSION; i++) {
    max_fram_data = max(max_fram_data, fram_lengths[i]);
    min_fram_data = min(min_fram_data, fram_lengths[i]);
  }

  fram = new I2C_eeprom(CY15E004J_I2C_ADDR, CY15E004J_DEVICE_SIZE, &Wire);
  if (!fram->begin()) {
    delete fram;
    fram = 0;
    Log.error("No onboard FRAM at address 0x%02X", CY15E004J_I2C_ADDR);
    return;
  }

  uint8_t *buf = (uint8_t *)&fram_data;

  Log.notice("Reading onboard FRAM");

  int len = fram->readBlock(0, buf, max_fram_data);
  if (len < min_fram_data) {
    Log.warning("Onboard FRAM incomplete (%d bytes read), initializing...", len);
    initalize_fram_data(CURRENT_FRAM_VERSION, buf);
    return;
  }

  bool empty = true;
  for (int j = 0; j < len; j++) {
    if (buf[j] != 0xFF) {
      empty = false;
      break;
    }
  }

  if (empty) {
    Log.warning("Onboard EEPROM is empty, initializing...");
    initalize_fram_data(CURRENT_FRAM_VERSION, buf);
    return;
  }

  // Check the checksum (use same checksum as EEPROMs)
  if (eeprom_checksum(buf, len)) {
    Log.warning("Onboard EEPROM has bad checksum, initializing...");
    initalize_fram_data(CURRENT_FRAM_VERSION, buf);
    return;
  }

  uint8_t version = fram_data.current.version;
  
  if (version > CURRENT_FRAM_VERSION || version < 1) {
    Log.error("Onboard FRAM has an unsupported version (%d)", version);
    delete fram;
    fram = 0;
    return;
  }

  if (len < fram_lengths[version - 1]) {
    Log.warning("Onboard FRAM incomplete for v%d (%d of %d bytes read), initializing...", version, len, fram_lengths[version - 1]);
    initalize_fram_data(CURRENT_FRAM_VERSION, buf);
    return;
  } 

  fram_dirty = false;
  Log.notice("Found v%d FRAM", version);

  if (version != CURRENT_FRAM_VERSION) {
    upgrade_fram_version(&fram_data);
  }
}

void update_fram(void)
{
  CoreMutex m(&fram_mutex);
  if (!fram_dirty) {
    return;
  }
  
  int version = fram_data.current.version;
  int len = fram_lengths[version - 1];
  int written = fram->writeBlock(0, (uint8_t *)&fram_data, len);

  if (written == len) {
    fram_dirty = false;
  }
}

int write_to_fram(uint8_t *buf, int len, int offset)
{
  CoreMutex m(&fram_mutex);

  int version = fram_data.current.version;
  int maxlen = fram_lengths[version - 1];
  uint8_t *target = (uint8_t *)&fram_data;

  if (len + offset >= maxlen) {
    return 0;
  }

  memcpy(&target[offset], buf, len);
  fram_dirty = true;
  return len;
}

void upgrade_fram_version(fram_data_t *data)
{
  CoreMutex m(&fram_mutex);

  Log.notice("Upgrading onboard FRAM contents from version %d to version %d", fram_data.current.version, CURRENT_FRAM_VERSION);
  /* Currently no upgrade paths to implement */
  fram_dirty = true;
}

void fram_lock(void)
{
  mutex_enter_blocking(&fram_mutex);
}

void fram_unlock(void)
{
  mutex_exit(&fram_mutex);  
}

void fram_clear_error_list(void)
{
  CoreMutex m(&fram_mutex);

  fram_data.current.error_list_count = 0;
  fram_dirty = true;
}

void fram_add_error(uint8_t code)
{
  CoreMutex m(&fram_mutex);

  beeper.register_beeper(1, 250, 250);

  int error_list_len = fram_data.current.error_list_count;

  int i;
  for (i = 0; i < error_list_len; i++) {
    if (fram_data.current.error_list[i].code == code) {
      break;
    }
  }

  int index = i;
  error_list_item_t *item = &fram_data.current.error_list[index];
  
  if (i == error_list_len) {
    if (error_list_len == MAX_ERROR_COUNT) {
      return;   // overflow... maybe should do LRU cache?
    }
    memset(item, 0x00, sizeof(error_list_item_t));
    item->code = code;
    fram_data.current.error_list_count++;
  }

  CoreMutex m1(&fsm_mutex);
  item->count++;
  item->status = 0x01;    // Stored
  item->state = (fsm_state << 8);
  item->temperature = (uint8_t)(((externalTempSensor->get_value() / 50) + 1) / 2 + 50);
  item->vbat = batteryVoltageSensor->get_value();
  memcpy((char *)&item->operating_time, (char *)&fram_data.current.total_working_duration, sizeof(time_sensor_t));

  fram_dirty = true;  
}

void fram_write_co2(uint8_t value)
{
  CoreMutex m(&fram_mutex);

  if (fram_data.current.minimum_co2 == 0x00) {
    fram_data.current.minimum_co2 = 0xFF;
  }
  fram_data.current.current_co2 = value;
  fram_data.current.minimum_co2 = min(value, fram_data.current.minimum_co2);
  fram_data.current.maximum_co2 = max(value, fram_data.current.maximum_co2);
}