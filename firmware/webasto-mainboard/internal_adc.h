#ifndef __internal_adc_h_
#define __internal_adc_h_

#include "analog_source.h"

class InternalADCSource : public AnalogSourceBase {
  public:
    InternalADCSource(int index, int channel, int bits, int mult = 0, int div_ = 0) :
        AnalogSourceBase(index, 100, 0x00, bits, mult, div_)
    {
      _channel = channel;
    };

    void init(void);
  protected:
    int32_t read_device(void);
    int32_t convert(int32_t reading);
    bool i2c_is_connected(void) { return true; };

    int _min_bits = 12;
    int _max_bits = 12;
    uint8_t _channel;
};


#endif
