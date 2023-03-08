#ifndef __internal_gpio_h_
#define __internal_gpio_h_

#include "sensor.h"

class InternalGPIODigitalSensor : public LocalSensor {
  public:
    InternalGPIODigitalSensor(int id, int pin, bool active_low = false) :
        LocalSensor(id, 1, 0, 1), _pin(pin), _active_low(active_low) { };

    void init(void);
  protected:
    int32_t get_raw_value(void);
    int32_t convert(int32_t reading);

    int _pin;
    bool _active_low;
};


#endif
