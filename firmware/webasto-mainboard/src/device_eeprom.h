#ifndef __device_eeprom_h_
#define __device_eeprom_h_

#include <Arduino.h>
#include <pico.h>

#define DEVICE_INFO_COUNT 12

typedef struct {
  uint8_t *buf;
  int len;
} device_info_t;

typedef uint16_t device_index_t[DEVICE_INFO_COUNT];

extern device_info_t device_info[DEVICE_INFO_COUNT];
extern device_index_t device_index;
extern const device_info_t default_device_info[DEVICE_INFO_COUNT];
extern bool device_info_dirty;

void init_device_eeprom(void);
void update_device_eeprom(void);
device_info_t *get_device_info(int index);
int get_device_info_string(int index, uint8_t *buf, int len);

#endif
