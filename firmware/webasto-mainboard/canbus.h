#ifndef __canbus_h_
#define __canbus_h_

#include <Arduino.h>

class CANBus {
  public:
    CANBus() { 
      _initialized = false;
    };

    bool begin(void);
    int write(int id, const char *buf, int len);
    int read(int *id, const char *buf, int len);
    bool available(void);

  protected:
    bool _initialized;
};

#define CANBUS_BUF_SIZE   64


#define CANBUS_ID_MAINBOARD 1
#define CANBUS_ID_WBUS      2


void init_canbus(void);
void update_canbus_rx(void);
void update_canbus_tx(void);
void canbus_send(int id, uint8_t *buf, int len);

#endif