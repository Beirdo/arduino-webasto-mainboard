#ifndef __ds2482_h_
#define __ds2482_h_

#include "analog_source.h"

class DS2482Source : AnalogSourceBase {
  public:
    virtual void init(void);
  protected:
    virtual int32_t read_device(void);
    void wait_for_1wire(uint32_t poll_us);
    virtual int32_t convert(int32_t reading);

    int _min_bits = 9;
    int _max_bits = 12;
    uint16_t _conv_ms;
  private:
    uint8_t _rom[8];
};

#endif
