#ifndef __sensor_eeprom_h_
#define __sensor_eeprom_h_

#include <Arduino.h>
#include <pico.h>
#include <assert.h>

#define PCA9501_DEVICE_SIZE     256
#define PCA9501_BLOCK_SIZE      16
#define PCA9501_I2C_GPIO_ADDR   0x30
#define PCA9501_I2C_EEPROM_ADDR 0x70

struct eeprom_v1_s {
  uint8_t version;
  uint8_t checksum;
  uint8_t board_num;
  uint8_t capabilities;
  uint8_t addr_pca9501_gpio;
  uint8_t addr_mcp96l01;
  uint8_t addr_ads7823;
  uint8_t addr_ds2482;
};

typedef union {
  struct eeprom_v1_s v1;
  struct eeprom_v1_s current;
} eeprom_data_t;

static_assert(sizeof(eeprom_data_t) <= PCA9501_DEVICE_SIZE, "Structure eeprom_data_t is larger than the EEPROM!");

#define CURRENT_EEPROM_VERSION  1
#define MAX_EEPROM_VERSION      1
extern int eeprom_lengths[];

#define CAPABILITIES_EXTERNAL_TEMP    0x01
#define CAPABILITIES_BATTERY_VOLTAGE  0x02
#define CAPABILITIES_COOLANT_TEMP     0x04
#define CAPABILITIES_EXHAUST_TEMP     0x08
#define CAPABILITIES_IGNITION_SOURCE  0x10

extern const char *capabilities_names[7];
extern eeprom_data_t eeprom_data[8];
extern void *eeprom_devices[8];

void init_eeprom(void);
uint8_t eeprom_checksum(uint8_t *buf, int len);
void upgrade_eeprom_version(eeprom_data_t *data);

#endif