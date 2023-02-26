#ifndef __ina219_h_
#define __ina219_h_

#include "sensor.h"

class INA219Sensor : public LocalSensor<uint16_t> {
  public:
    INA219Sensor(int id, uint8_t i2c_address, int bits, volatile bool *enable) :
      LocalSensor<uint16_t>(id, 0xFFFF, 20, bits, i2c_address), 
      _enable_signal(enable)
    {};

    void init(void);    
  protected:
    uint16_t get_raw_value(void);
    uint16_t convert(uint16_t reading);
    void _do_feedback(void);

    int _min_bits = 9;
    int _max_bits = 12;
    uint16_t _device_config;
    int _conv_us;
    volatile bool *_enable_signal;
};


#endif
