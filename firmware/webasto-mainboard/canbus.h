#ifndef __canbus_h_
#define __canbus_h_

#include <Arduino.h>
#include <canbus_ids.h>

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


void init_canbus(void);
void update_canbus_rx(void);
void update_canbus_tx(void);
void canbus_send(int id, uint8_t *buf, int len);

template <typename T>
void canbus_output_value(int id, T value)
{
  int len = sizeof(value);
  uint8_t buf[len];

  memcpy(buf, &value, len);
  canbus_send(id, buf, len);
}

void canbus_request_value(int id);


#endif