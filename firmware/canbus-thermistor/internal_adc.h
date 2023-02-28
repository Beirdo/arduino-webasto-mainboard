#include <sys/_stdint.h>
#ifndef __internal_adc_h_
#define __internal_adc_h_

#include <sensor.h>

class InternalADCSensor : public LocalSensor {
  public:
    InternalADCSensor(int id, int channel, int bits, int mult = 0, int div_ = 0, int32_t (*convertfunc)(int32_t) = 0) :
      LocalSensor(id, 2, 100, bits, mult, div_)
    {
      _channel = channel;
      _convertfunc = convertfunc;
      _connected = true;
    };

    void init(void);
  protected:
    int32_t get_raw_value(void);
    int32_t convert(int32_t reading);
    void do_feedback(void); 

    int _bits_allowed[3] = {8, 10, 12};
    uint8_t _channel;
    int32_t (*_convertfunc)(int32_t);
    int32_t _vref = -1;
    int32_t _ts_cal1;
    int32_t _ts_cal2;
    int32_t _ts_cal1_temp = 30000;
    int32_t _ts_cal2_temp = 110000;
    int32_t _vrefint_cal;
};


#endif
