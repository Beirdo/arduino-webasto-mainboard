#ifndef __analog_h
#define __analog_h

#include <arduino.h>
#include <pico.h>

void init_analog(void);
void update_analog(void);

int32_t adc_scale_reading(int index, uint16_t reading, uint16_t multiplicand, uint16_t divisor);
int32_t adc_filter_readings(int32_t *readings);
void adc_append_value(int index, int32_t value);
void adc_write_register(uint8_t regnum, uint8_t value) ;
void adc_read_conversions(void);
uint16_t adc_read_channel(int index);

int32_t get_vbat_mv(void);
int32_t get_coolant_temp_centiC(void);
int32_t get_exhaust_temp_centiC(void);
int32_t get_glow_plug_mv(void);
int32_t get_int_temp_centiC(void);


#endif
