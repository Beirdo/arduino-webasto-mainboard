#ifndef __internal_adc_h_
#define __internal_adc_h_

#include "sensor.h"

class InternalADCSensor : public LocalSensor<int16_t> {
  public:
    InternalADCSensor(int id, int channel, int bits, int mult = 0, int div_ = 0) :
      LocalSensor<int16_t>(id, 0x8000, 100, bits, mult, div_)
    {
      _channel = channel;
      _connected = true;
    };

    void init(void);
  protected:
    int16_t get_raw_value(void);
    int16_t convert(int16_t reading);
    void _do_feedback(void); 

    int _min_bits = 12;
    int _max_bits = 12;
    uint8_t _channel;
};


#endif
