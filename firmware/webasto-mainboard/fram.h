#ifndef __fram_h_
#define __fram_h_

#include <Arduino.h>
#include <pico.h>
#include <assert.h>

#include <I2C_eeprom.h>

#define CY15E004J_DEVICE_SIZE 512
#define CY15E004J_BLOCK_SIZE 8
#define CY15E004J_I2C_ADDR 0x50

typedef struct time_sensor_s {
  uint16_t hours;
  uint8_t minutes;
} time_sensor_t;

typedef struct {
  uint8_t code;
  uint8_t count;  
  uint8_t status;
  uint16_t state;
  uint8_t temperature;  // degC with 50C offset
  uint16_t vbat;
  time_sensor_t operating_time;
} error_list_item_t;

#define MAX_ERROR_COUNT   12

struct fram_v1_s {
  uint8_t version;
  uint8_t checksum;
  time_sensor_t burn_duration_parking_heater[4];
  time_sensor_t burn_duration_supplemental_heater[4];
  time_sensor_t working_duration_parking_heater;
  time_sensor_t working_duration_supplemental_heater;
  uint16_t start_counter_parking_heater;
  uint16_t start_counter_supplemental_heater;
  uint16_t counter_emergency_shutdown;
  time_sensor_t total_burn_duration;
  time_sensor_t total_working_duration;
  uint16_t total_start_counter;
  uint8_t error_list_count;
  error_list_item_t error_list[MAX_ERROR_COUNT];
  uint8_t device_status;
  uint8_t current_co2;
  uint8_t minimum_co2;
  uint8_t maximum_co2;
  uint8_t lockdown;
};

typedef union {
  struct fram_v1_s v1;
  struct fram_v1_s current;
} fram_data_t;

extern int fram_lengths[];

#define CURRENT_FRAM_VERSION 1
#define MAX_FRAM_VERSION 1

static_assert(sizeof(fram_data_t) <= CY15E004J_DEVICE_SIZE, "Structure fram_data_t is larger than the FRAM!");

extern fram_data_t fram_data;
extern bool fram_dirty;
extern I2C_eeprom *fram;
extern mutex_t fram_mutex;

void init_fram(void);
int write_to_fram(uint8_t *buf, int len, int offset);
void update_fram(void);
void upgrade_fram_version(fram_data_t *data);

void fram_lock(void);
void fram_unlock(void);
void fram_clear_error_list(void);
void fram_add_error(uint8_t code);
void fram_write_co2(uint8_t value);

#endif