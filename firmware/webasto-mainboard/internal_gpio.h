#ifndef __internal_gpio_h_
#define __internal_gpio_h_

#include "analog_source.h"

class InternalGPIODigitalSource : public AnalogSourceBase {
  public:
    InternalGPIODigitalSource(int pin, bool active_low = true) : 
        AnalogSourceBase(0, 1), _pin(pin), _active_low(active_low) { };
    
    void init(void);
    void feedback(int index);
  protected:
    int32_t read_device(void);
    int32_t convert(int32_t reading);

    int _pin;
    bool _active_low;
};


#endif