#ifndef __display_h_
#define __display_h_

#include <Arduino.h>
#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>
#include <stdlib.h>
#include <string.h>

#include "analog_source.h"

class Display {
  public:
    Display(uint8_t i2c_address, int cols, int rows);
    void update();
    void log();
    void clearLine(int y);
    void printHexByte(int x, int y, uint8_t data);
    void printWatts(int x, int y, int burnPower);
    void printPercent(int x, int y, int percent);
    void printTemperature(int x, int y, int millivolts);
    void printMilliohms(int x, int y, int milliohms);
    void printLabel(int x, int y, const char *str);

  protected:
    virtual void setCursor(int x, int y) = 0;
    virtual void write(uint16_t ch) = 0;
    virtual void display(void) = 0;

    uint16_t getHexDigit(uint8_t nibble);
    inline int getOffset(int x, int y) { return y * _columns + x; };
    inline bool isOffsetInRow(int offset, int y) 
    { 
      int _min = getOffset(0, y);
      int _max = getOffset(_columns, y);
      return offset >= _min && offset < _max;
    }
    void printDigits(int x, int y, int value, int count, uint8_t suffix = 0x00, bool nonZero = false);

    uint8_t _i2c_address;
    int _columns;
    int _rows;
    uint16_t *_cache;
    uint16_t *_display;
    bool *_dirty;
    bool _connected;
    mutex_t _mutex;
};

extern Display *display;

void init_display(void);
void update_display(void);

#endif