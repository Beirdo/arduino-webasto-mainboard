#ifndef __ina219_h_
#define __ina219_h_

#include "analog_source.h"

class INA219Source : public AnalogSourceBase {
  public:
    INA219Source(uint8_t i2c_address, int bits, int rshunt, int max_current) : 
        AnalogSourceBase(i2c_address, bits, 4) 
    {
      _rshunt = rshunt;
      _max_current = max_current;
    };
    
    void init(void); 
  protected:
    void read_device(void);
    int32_t convert(int index);

    int _min_bits = 9;
    int _max_bits = 12;
    uint16_t _device_config;
    int _conv_us;
    int _rshunt;
    int _max_current;
};


#endif
