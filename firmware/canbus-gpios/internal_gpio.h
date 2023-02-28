#include <sys/_stdint.h>
#ifndef __internal_adc_h_
#define __internal_adc_h_

#include <sensor.h>

class InternalGPIOSensor : public LocalSensor {
  public:
    InternalGPIOSensor(int id, int pin) :
      LocalSensor(id, 1, 1, 1)
    {
      _pin = pin;
      _connected = true;
    };

    void init(void);
  protected:
    int32_t get_raw_value(void);
    int32_t convert(int32_t reading);
    void do_feedback(void); 

    int _pin;
};


#endif
