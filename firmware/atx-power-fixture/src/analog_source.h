#ifndef __analog_source_
#define __analog_source_

#include <Arduino.h>

#define ADC_AVG_WINDOW          16
#define UNUSED_READING          ((int32_t)(0x80000000))
#define DISABLED_READING        ((int32_t)(0x80000001))

typedef struct {
  int raw_value;
  int readings[ADC_AVG_WINDOW];
  int tail;
  int prev_value;
  int value;
} readings_t;

class AnalogSourceBase {
  public:
    AnalogSourceBase(uint8_t i2c_address, int bits, int count = 1, int mult = 0, int div_ = 0);
    ~AnalogSourceBase(void);
    virtual void init(void) = 0;
    void update(void);
    inline int32_t get_value(int index) {
      if (index < 0 || index > _reading_count) {
        return 0;
      } 

      int32_t value = _data[index].value;
      if (value == UNUSED_READING || value == DISABLED_READING) {
        return 0;
      }

      return value;
    };

  protected:
    bool _valid;
    uint8_t _i2c_address;
    uint8_t _bits;
    uint8_t _bytes;
    
    readings_t *_data;
    int _reading_count;

    int _mult;
    int _div;
    bool _connected;
    char *_classname;

    virtual void read_device(void) = 0;
    virtual int32_t convert(int index);
    int32_t filter(int index);
    void append_value(int index, int32_t value);

    void i2c_write_register(uint8_t regnum, uint8_t value, bool skip_byte = false);
    void i2c_write_register_word(uint8_t regnum, uint16_t value);
    void i2c_read_data(uint8_t regnum, uint8_t *buf, uint8_t count, bool skip_regnum = false);
    virtual bool i2c_is_connected(void);
};

#endif
