#ifndef __dummy_h_
#define __dummy_h_

#include "analog_source.h"

class DummySource : public AnalogSourceBase {
  public:
    DummySource(int index) : AnalogSourceBase(index, 0, 0xFF, 16) {};
    void init(void);
  protected:
    int32_t read_device(void);
    int32_t convert(int32_t reading);
};

#endif
