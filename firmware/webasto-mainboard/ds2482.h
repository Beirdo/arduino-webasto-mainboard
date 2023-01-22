#ifndef __ds2482_h_
#define __ds2482_h_

#include "analog_source.h"

class DS2482Source : public AnalogSourceBase {
  public:
    DS2482Source(uint8_t i2c_address, int bits) : AnalogSourceBase(i2c_address, bits) {};
    void init(void);
    void feedback(int index);
  protected:
    int32_t read_device(void);
    void wait_for_1wire(uint32_t poll_us);
    int32_t convert(int32_t reading);

    int _min_bits = 9;
    int _max_bits = 12;
    uint16_t _conv_ms;
  private:
    uint8_t _rom[8];
};

#endif
