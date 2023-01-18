#include <pico.h>
#include <Wire.h>

#include "analog.h"
#include "ds2482.h"


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

DS2482Source externalTempSensor(0x18, 9, 0, 0);

void init_analog(void)
{
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.begin();
  Wire.setClock(400000);

  externalTempSensor.init();
}

void update_analog(void) 
{
  externalTempSensor.update();
}
