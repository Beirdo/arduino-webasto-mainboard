#ifndef __ds2482_h_
#define __ds2482_h_

#include "sensor.h"

class DS2482Sensor : public LocalSensor {
  public:
    DS2482Sensor(int id, uint8_t i2c_address, int bits) :
      LocalSensor(id, 2, 100, bits, i2c_address) {};
    void init(void);
  protected:
    int32_t get_raw_value(void);
    void wait_for_1wire(uint32_t poll_us);
    int32_t convert(int32_t reading);

    int _min_bits = 9;
    int _max_bits = 12;
    uint16_t _conv_ms;
  private:
    uint8_t _rom[8];
};

#endif
