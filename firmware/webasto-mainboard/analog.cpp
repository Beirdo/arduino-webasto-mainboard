#include <pico.h>
#include <Wire.h>

#include "analog.h"

enum {
  ADC_CHAN_VBAT = 0,
  ADC_CHAN_COOLANT_TEMP,
  ADC_CHAN_EXHAUST_TEMP,
  ADC_CHAN_GLOW_PLUG,
  ADC_CHAN_INT_TEMP,
  ADC_CHAN_COUNT,
};

#define ADC_AVG_WINDOW          16
#define PIN_ADC_BUSY            6
#define PIN_ADC_START           7
#define ADC_I2C_ADDRESS         0x40

int32_t adc_scaled_values[ADC_CHAN_COUNT][ADC_AVG_WINDOW];
int32_t adc_valid_readings[ADC_CHAN_COUNT];
int adc_tail[ADC_CHAN_COUNT];
int32_t adc_reading[ADC_CHAN_COUNT];
const uint16_t adc_mult[ADC_CHAN_GLOW_PLUG + 1] = {
  19767, // 0-4096 ADC -> 0-3.3V -> 0-19767mV
  13749, // 0-3724 ADC -> 0-3.0V -> 0-12500cC (degC/100)
  38749, // 0-3724 ADC -> 0-3.0V -> 0-35000cC (degC/100)
  19767, // 0-4096 ADC -> 0-3.3V -> 0-19767mV
};
const uint16_t adc_div[ADC_CHAN_COUNT] = {
  4096,  
  4096,
  4096,
  4096,
};
uint16_t adc_raw_buffer[4];

void init_analog(void)
{
  pinMode(PIN_ADC_BUSY, INPUT);
  digitalWrite(PIN_ADC_START, LOW);
  pinMode(PIN_ADC_START, OUTPUT);
  
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.begin();
  Wire.setClock(400000);

  for (int i = 0; i < ADC_CHAN_COUNT; i++) {
    for (int j = 0; j < ADC_AVG_WINDOW; j++) {
      adc_scaled_values[i][j] = 0;
    }
    adc_valid_readings[i] = 0x7FFFFFFF;
    adc_tail[i]= 0;
    adc_reading[i] = 0;
  }
}

int32_t adc_scale_reading(int index, uint16_t reading, uint16_t multiplicand, uint16_t divisor) 
{
  int32_t value = 0;
  if (index < 0 || index >= ADC_CHAN_COUNT) {
    return value;
  }

  if (index <= ADC_CHAN_GLOW_PLUG) {
    value = reading * multiplicand;
    value /= divisor;
    return value;
  }

  // ADC_CHAN_INT_TEMP is already converted.
  return (int32_t)reading;
}

int32_t adc_filter_readings(int32_t *readings)
{
  int32_t accumulator = 0;  
  int count = 0;
  int32_t min_reading = (int32_t)0xFFFFFFFF;
  int min_index = -1;
  int32_t max_reading = (int32_t)0x7FFFFFFF;
  int max_index = -1;
  int i;

  for (i = 0; i < ADC_AVG_WINDOW; i++) {
    int32_t value = readings[i];
    if (value != 0x7FFFFFFF && value < min_reading) {
      min_reading = value;
      min_index = i;
    }

    if (value != 0x7FFFFFFF && value > max_reading) {
      max_reading = value;
      max_index = i;
    }
  }

  for (i = 0, count = 0; i < ADC_AVG_WINDOW; i++) {
    int32_t value = readings[i];
    if (value != 0x7FFFFFFF && i != min_index && i != max_index) {
      accumulator += value;
      count++;
    }
  }

  if (!count) {
    return 0;
  }

  return accumulator / count;
}

void adc_append_value(int index, int32_t value) 
{
  adc_valid_readings[adc_tail[index]] = value;
  adc_tail[index] += 1;
  adc_tail[index] %= ADC_AVG_WINDOW;
}

void adc_write_register(uint8_t regnum, uint8_t value) 
{
  Wire.beginTransmission(ADC_I2C_ADDRESS);
  Wire.write(regnum);
  Wire.write(value);
  Wire.endTransmission();  
}

void adc_read_conversions(void) 
{
  uint8_t *buf = (uint8_t *)&adc_raw_buffer;
  int len = 4 * sizeof(uint16_t);

  while (digitalRead(PIN_ADC_BUSY) == LOW) {
    delayMicroseconds(2);
  }
  
  Wire.beginTransmission(ADC_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom(ADC_I2C_ADDRESS, len);

  for (int i = 0; i < len && Wire.available(); i++) {
    *(buf++) = Wire.read();
  }
}

uint16_t adc_read_channel(int index) 
{
  if (index < 0 || index >= ADC_CHAN_COUNT) {
    return 0;
  }

  if (index == 0) {
    adc_write_register(0x02, 0xF6);  // sequence all 4 inputs, use BUSY, active low
    digitalWrite(PIN_ADC_START, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_ADC_START, LOW);
    adc_read_conversions();
  }
  
  if (index <= ADC_CHAN_GLOW_PLUG) {
    return adc_raw_buffer[index];
  }

  // ADC_CHAN_INT_TEMP
  float temperature = analogReadTemp(2.048);
  temperature *= 100.0;
  return (uint16_t)((int)temperature & 0xFFFF);
}

void update_analog(void) 
{
  uint16_t raw_value;
  int32_t scaled_value;
  
  for (int i = 0; i <= ADC_CHAN_GLOW_PLUG; i++) {
    raw_value = adc_read_channel(i);
    scaled_value = adc_scale_reading(i, raw_value, adc_mult[i], adc_div[i]);
    adc_append_value(i, scaled_value);
    adc_reading[i] = adc_filter_readings(&adc_valid_readings[i]);
  }
}

int32_t get_vbat_mv(void)
{
  return adc_reading[ADC_CHAN_VBAT];
}

int32_t get_coolant_temp_centiC(void)
{
  return adc_reading[ADC_CHAN_COOLANT_TEMP];
}

int32_t get_exhaust_temp_centiC(void)
{
  return adc_reading[ADC_CHAN_EXHAUST_TEMP];
}

int32_t get_glow_plug_mv(void)
{
  return adc_reading[ADC_CHAN_GLOW_PLUG];
}

int32_t get_int_temp_centiC(void)
{
  return adc_reading[ADC_CHAN_INT_TEMP];
}
