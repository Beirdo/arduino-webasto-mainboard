#ifndef __dummy_h_
#define __dummy_h_

#include "analog_source.h"

class DummySource : public AnalogSourceBase {
  public:
    DummySource(int index) : AnalogSourceBase(index, 0xFF, 16) {};
    void init(void);
    void feedback(int index);
  protected:
    int32_t read_device(void);
    int32_t convert(int32_t reading);
};

#endif
