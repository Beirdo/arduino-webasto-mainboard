#include <Arduino.h>
#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>
#include <SerLCD.h>
#include <stdlib.h>
#include <string.h>
#include <CoreMutex.h>

#include "display.h"
#include "project.h"
#include "fsm.h"
#include "analog.h"
#include "fuel_pump.h"

Display::Display(uint8_t i2c_address) : _i2c_address(i2c_address) 
{
  Wire.begin();
  Wire.beginTransmission(_i2c_address);
  _connected = (Wire.endTransmission() == 0);

  memset(&_cache, 0x20, MAX_ROWS * MAX_COLUMNS);
  memset(&_display, 0x20, MAX_ROWS * MAX_COLUMNS);

  for (int i = 0; i < MAX_ROWS; i++) {
    _cache[i][MAX_COLUMNS] = 0;
    _display[i][MAX_COLUMNS] = 0;
  }

  _dirty = false;

  mutex_init(&_mutex);

  if (!_connected) {
    Log.warning("No SerLCD connected at I2C0/0x%X", _i2c_address);
    return;
  }

  Log.notice("Found SerLCD at I2C0/0x%X", _i2c_address);
  _lcd.begin(Wire, _i2c_address);
  _lcd.noDisplay();
  _lcd.noBlink();
  _lcd.noCursor();
  _lcd.clear();
  _lcd.home();
  _lcd.display();
  _lcd.enableSystemMessages();
}

void Display::update(void)
{
  if (!_connected || !_dirty) {
    return;
  }

  CoreMutex m(&_mutex);
  int cursorX, cursorY;
  int x, y;

  for (y = 0; y < MAX_ROWS; y++) {
    for (x = 0; x < MAX_COLUMNS; x++) {
      if (_cache[y][x] != _display[y][x]) {
        if (cursorX != x || cursorY != y) {
          _lcd.setCursor(x, y);
          cursorX = x;
          cursorY = y;
        }

        _lcd.write(_cache[y][x]);
        _display[y][x] = _cache[y][x];
        cursorX++;
      }
    }
  }
  _dirty = false;
}

void Display::clearLine(int y)
{
  CoreMutex m(&_mutex);

  memset(&_cache[y], 0x20, MAX_COLUMNS);
  _dirty = true;
}

uint8_t Display::printHexNibble(uint8_t nibble)
{
  nibble &= 0x0F;

  if (nibble < 10) {
    return (0x30 + nibble);
  } else {
    return (0x41 + nibble - 10);
  }
}

void Display::printState(int x, int y, uint8_t state)
{
  if (y < 0 || y >= MAX_ROWS || x < 0) {
    return;
  }

  CoreMutex m(&_mutex);

  if (x < MAX_COLUMNS) {
    _cache[y][x++] = printHexNibble(HI_NIBBLE(state));
    _dirty = true;
  }

  if (x < MAX_COLUMNS) {
    _cache[y][x++] = printHexNibble(LO_NIBBLE(state));
    _dirty = true;
  }
}

void Display::printDigits(int x, int y, int value, int count, uint8_t suffix, bool nonZero)
{
  if (y < 0 || y >= MAX_ROWS || x < 0 || count < 0 || count > 6) {
    return;
  }

  bool negative = (value < 0);

  value = abs(value);
  if (negative) {
    count--;
    if (x < MAX_COLUMNS) {
      _cache[y][x++] = ' ';
      _dirty = true;
    }
  }
  
  int limit = 1;
  for (int i = 0; i < count - 1; i++) {
    limit *= 10;
  }

  for (int i = limit; i != 0 && x < MAX_COLUMNS; i /= 10, x++) {
    int digit = (value / i) % 10;
    if (digit || nonZero || i == 1) {
      // skip leading zeros.
      if (digit) {
        if (!nonZero && negative) {
          _cache[y][x - 1] = '-';
          _dirty = true;
        }

        nonZero = true;
      }

      _cache[y][x] = 0x30 + digit;
      _dirty = true;
    } else {
      _cache[y][x] = ' ';
      _dirty = true;
    }
  }

  if (suffix) {
    if (x < MAX_COLUMNS) {
      _cache[y][x++] = suffix;
      _dirty = true;
    }
  }
}

void Display::printWatts(int x, int y, int burnPower)
{
  CoreMutex m(&_mutex);

  printDigits(x, y, burnPower, 4, 'W');
}

void Display::printPercent(int x, int y, int percent)
{
  CoreMutex m(&_mutex);

  printDigits(x, y, percent, 3, '%');
}

void Display::printTemperature(int x, int y, int temperature)
{
  CoreMutex m(&_mutex);

  printDigits(x, y, temperature / 100, 4, '.');
  printDigits(x + 5, y, abs(temperature) % 100, 2, 'C', true);
}

void Display::printMilliohms(int x, int y, int milliohms)
{
  CoreMutex m(&_mutex);

  printDigits(x, y, milliohms, 5, 'm');
}
    

void Display::printLabel(int x, int y, const char *str)
{
  if (y < 0 || y >= MAX_ROWS || x < 0 || !str) {
    return;
  }

  CoreMutex m(&_mutex);

  for ( ; *str && x < MAX_COLUMNS; str++, x++) {
    _cache[y][x] = *str;
    _dirty = true;
  }
}

void Display::log(void) 
{
  CoreMutex m(&_mutex);
  for (int i = 0; i < MAX_ROWS; i++) {
    Log.notice("Line %d - |%s|", i, _cache[i]);
  }
}


Display *display = 0;

void init_display(void) {
  display = new Display(I2C_ADDR_SERLCD);
}

void update_display(void) {
  CoreMutex m(&fsm_mutex);

  Log.notice("Starting display->update");

  int state = fsm_state;
  display->printLabel(0, 0, "State:");
  display->printState(7, 0, state);
 
  display->printLabel(10, 0, "P:");
  display->printWatts(13, 0, fuelPumpTimer.getBurnPower());
 
  display->printLabel(0, 1, "Fl:");
  display->printMilliohms(4, 1, flameDetectorSensor->get_value());
 
  display->printLabel(10, 1, "Fan:");
  display->printPercent(15, 1, combustionFanPercent);
 
  int temp = internalTempSensor->get_value();
  display->printLabel(0, 2, "I:");
  display->printTemperature(2, 2, temp);
 
  temp = externalTempSensor->get_value();
  display->printLabel(10, 2, "O:");
  display->printTemperature(12, 2, temp);
 
  temp = coolantTempSensor->get_value();
  display->printLabel(0, 3, "C:");
  display->printTemperature(2, 3, temp);
 
  temp = exhaustTempSensor->get_value();
  display->printLabel(10, 3, "E:");
  display->printTemperature(12, 3, temp);
 
  display->update();
  display->log();
}