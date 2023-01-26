#ifndef __display_h_
#define __display_h_

#include <Arduino.h>
#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>
#include <SerLCD.h>
#include <stdlib.h>
#include <string.h>

#include "analog_source.h"

class Display {
  public:
    Display(uint8_t i2c_address);
    void update();
    void log();
    void clearLine(int y);
    void printState(int x, int y, uint8_t state);
    void printWatts(int x, int y, int burnPower);
    void printPercent(int x, int y, int percent);
    void printTemperature(int x, int y, int millivolts);
    void printMilliohms(int x, int y, int milliohms);
    void printLabel(int x, int y, const char *str);

  protected:
    uint8_t printHexNibble(uint8_t nibble);
    void printDigits(int x, int y, int value, int count, uint8_t suffix = 0x00, bool nonZero = false);

    SerLCD _lcd;
    uint8_t _i2c_address;
    uint8_t _cache[MAX_ROWS][MAX_COLUMNS + 1];
    uint8_t _display[MAX_ROWS][MAX_COLUMNS + 1];
    bool _dirty;
    bool _connected;
    mutex_t _mutex;
};

extern Display *display;

void init_display(void);
void update_display(void);

#endif