#ifndef __analog_source_
#define __analog_source_

#include <Arduino.h>
#include <pico.h>

#define ADC_AVG_WINDOW          16
#define UNUSED_READING          ((int32_t)(0x80000000))

class AnalogSourceBase {
  public:
    AnalogSourceBase(uint8_t i2c_address, int bits, uint16_t mult, uint16_t div_);
    virtual void init(void) = 0;
    void update(void);
    inline int32_t get_value(void) { return _value; };

  protected:
    bool _valid;
    uint8_t _i2c_address;
    uint8_t _bits;
    uint8_t _bytes;
    int32_t _readings[ADC_AVG_WINDOW];
    int _tail;
    int32_t _value;
    mutex_t _i2c_mutex;
    uint16_t _mult;
    uint16_t _div;

    virtual int32_t read_device(void) = 0;
    virtual int32_t convert(int32_t reading);
    int32_t filter(void);
    void append_value(int32_t value);

    void i2c_write_register(uint8_t regnum, uint8_t value, bool skip_byte = false);
    void i2c_read_data(uint8_t regnum, uint8_t *buf, uint8_t count, bool skip_regnum = false);
};

#endif
