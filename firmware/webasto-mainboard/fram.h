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

struct fram_v1_s {
  uint8_t version;
  uint8_t checksum;
  uint8_t serial_num[32];
  time_sensor_t burn_duration_parking_heater[4];
  time_sensor_t burn_duration_supplemental_heater[4];
  time_sensor_t working_duration_parking_heater[4];
  time_sensor_t working_duration_supplemental_heater[4];
  uint16_t start_counter_parking_heater;
  uint16_t start_counter_supplemental_heater;
  uint16_t counter_emergency_shutdown;
  time_sensor_t total_burn_duration;
  time_sensor_t total_working_duration;
  uint16_t total_start_counter;
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

void init_fram(void);
int write_to_fram(uint8_t *buf, int len, int offset);
void update_fram(void);
void upgrade_fram_version(fram_data_t *data);

#endif