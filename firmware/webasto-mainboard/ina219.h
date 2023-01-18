#ifndef __ina219_h_
#define __ina219_h_

#include "analog_source.h"

class INA219Source : public AnalogSourceBase {
  public:
    INA219Source(uint8_t i2c_address, int bits, volatile bool *enable) : 
        AnalogSourceBase(i2c_address, bits) 
    {
      _enable_signal = enable;
    };
    
    virtual void init(void); 
  protected:
    virtual int32_t read_device(void);
    virtual int32_t convert(int32_t reading);

    int _min_bits = 9;
    int _max_bits = 12;
    uint16_t _device_config;
    int _conv_us;
    volatile bool *_enable_signal;
};


#endif