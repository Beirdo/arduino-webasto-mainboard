#ifndef __pca9501_h_
#define __pca9501_h_

#include "analog_source.h"

class PCA9501DigitalSource : public AnalogSourceBase {
  public:
    PCA9501DigitalSource(uint8_t i2c_address, uint8_t bitmask) : 
        AnalogSourceBase(i2c_address, 1) {_bitmask = bitmask;};
    
    void init(void);
    void feedback(int index);
  protected:
    int32_t read_device(void);
    int32_t convert(int32_t reading);
    uint8_t _bitmask;
};



#endif
