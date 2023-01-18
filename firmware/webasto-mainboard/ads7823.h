#ifndef __ads7823_h_
#define __ads7823_h_

#include "analog_source.h"

class ADS7823Source : public AnalogSourceBase {
  public:
    ADS7823Source(uint8_t i2c_address, int bits, int mult, int div_) : AnalogSourceBase(i2c_address, bits, mult, div_) {};
    virtual void init(void);
  protected:
    virtual int32_t read_device(void);

    int _min_bits = 12;
    int _max_bits = 12;
};


#endif