#ifndef __ads7823_h_
#define __ads7823_h_

#include "analog_source.h"

class ADS7823Source : public AnalogSourceBase {
  public:
    ADS7823Source(int index, uint8_t i2c_address, int bits, int mult, int div_) :
      AnalogSourceBase(index, i2c_address, bits, mult, div_) {};
    void init(void);
    void feedback(int index);
  protected:
    int32_t read_device(void);

    int _min_bits = 12;
    int _max_bits = 12;
};


#endif
