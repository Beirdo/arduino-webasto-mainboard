#ifndef __mcp96l01_h_
#define __mcp96l01_h_

#include "analog_source.h"

typedef enum {
  TYPE_K = 0,
  TYPE_J,
  TYPE_T,
  TYPE_N,
  TYPE_S,
  TYPE_E,
  TYPE_B,
  TYPE_R,
  TYPE_COUNT,
} thermocoupleType_t;

class MCP96L01Source : public AnalogSourceBase {
  public:
    MCP96L01Source(int index, uint8_t i2c_address, int bits, thermocoupleType_t type, int filter_bits = 4) : 
        AnalogSourceBase(index, i2c_address, bits)
    {
      _type = type;
      _filter_bits = filter_bits;
    };
    
    void init(void);
    void feedback(int index);
  protected:
    int32_t read_device(void);
    int32_t convert(int32_t reading);

    int _min_bits = 12;
    int _max_bits = 18;
    uint16_t _conv_ms;
    thermocoupleType_t _type;
    uint8_t _filter_bits;
    uint8_t _device_config;
};


#endif
