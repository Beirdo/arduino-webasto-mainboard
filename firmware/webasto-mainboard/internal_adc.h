#ifndef __internal_adc_h_
#define __internal_adc_h_

#include "sensor.h"

class InternalADCSensor : public LocalSensor {
  public:
    InternalADCSensor(int id, int channel, int bits, int mult = 0, int div_ = 0) :
      LocalSensor(id, 2, 100, bits, mult, div_)
    {
      _channel = channel;
      _connected = true;
    };

    void init(void);
  protected:
    int32_t get_raw_value(void);
    int32_t convert(int32_t reading);
    void do_feedback(void); 

    int _min_bits = 12;
    int _max_bits = 12;
    uint8_t _channel;
};


#endif
