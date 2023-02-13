#ifndef __ads7823_h_
#define __ads7823_h_

#include "analog.h"
#include "analog_source.h"
#include "ntc_thermistor.h"

class ADS7823Source : public AnalogSourceBase {
  public:
    ADS7823Source(int index, uint8_t i2c_address, int bits, int mult, int div_);
    ~ADS7823Source(void);
    void init(void);
  protected:
    int32_t read_device(void);
    int32_t convert(int32_t reading);

    int _min_bits = 12;
    int _max_bits = 12;
    NTCThermistor *_thermistor = 0;
};


#endif
