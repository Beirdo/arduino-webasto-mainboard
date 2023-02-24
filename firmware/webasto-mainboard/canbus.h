#ifndef __canbus_h_
#define __canbus_h_

#include <Arduino.h>

class CANBus {
  public:
    CANBus() { 
      _initialized = false;
    };

    bool begin(void);
    int write(const char *buf, int len);
    int read(const char *buf, int len);
    bool available(void);

  protected:
    bool _initialized;
}

#endif