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

Display::Display(uint8_t i2c_address, int columns, int rows) : 
  _i2c_address(i2c_address), _columns(columns), _rows(rows) 
{
  Wire.begin();
  Wire.beginTransmission(_i2c_address);
  _connected = (Wire.endTransmission() == 0);

  int len = _columns * _rows;
  _cache = new uint16_t[len];
  _display = new uint16_t[len];
  _dirty = new bool[_rows];

  mutex_init(&_mutex);

  for (int i = 0; i < _rows; i++ ) {
    clearLine(i);
    _dirty[i] = false;
  }

  memcpy(_display, _cache, len * 2);

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

  for (y = 0; y < _rows; y++) {
    if (!_dirty[y]) {
      continue;
    }

    for (x = 0; x < _columns; x++) {
      int offset = getOffset(x, y);
      if (_cache[offset] != _display[offset]) {
        if (cursorX != x || cursorY != y) {
          _lcd.setCursor(x, y);
          cursorX = x;
          cursorY = y;
        }

        _lcd.write(_cache[offset]);
        _display[offset] = _cache[offset];
        cursorX++;
      }
    }

    _dirty[y] = false;    
  }
}

void Display::clearLine(int y)
{
  CoreMutex m(&_mutex);

  uint16_t *buf = &_cache[getOffset(0, y)];

  for (int i = 0; i < _columns; i++) {
    *(buf++) = 0x0020;
  }
  _dirty[y] = true;
}

uint16_t Display::getHexDigit(uint8_t nibble)
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
  if (y < 0 || y >= _rows || x < 0) {
    return;
  }

  CoreMutex m(&_mutex);

  int offset = getOffset(x, y);
  if (isOffsetInRow(offset, y)) {
    _cache[offset++] = getHexDigit(HI_NIBBLE(state));
    _dirty[y] = true;
  }

  if (isOffsetInRow(offset, y)) {
    _cache[offset++] = getHexDigit(LO_NIBBLE(state));
    _dirty[y] = true;
  }
}

void Display::printDigits(int x, int y, int value, int count, uint8_t suffix, bool nonZero)
{
  if (y < 0 || y >= MAX_ROWS || x < 0 || count < 0 || count > 6) {
    return;
  }

  int offset = getOffset(x, y);
  bool negative = (value < 0);

  value = abs(value);
  if (negative) {
    count--;
    if (isOffsetInRow(offset, y)) {
      _cache[offset++] = (uint16_t)' ';
      _dirty[y] = true;
    }
  }
  
  int limit = 1;
  for (int i = 0; i < count - 1; i++) {
    limit *= 10;
  }

  for (int i = limit; i != 0 && isOffsetInRow(offset, y); i /= 10, offset++) {
    int digit = (value / i) % 10;
    if (digit || nonZero || i == 1) {
      // skip leading zeros.
      if (digit) {
        if (!nonZero && negative) {
          _cache[offset - 1] = (uint16_t)'-';
          _dirty[y] = true;
        }

        nonZero = true;
      }

      _cache[offset] = (uint16_t)(0x30 + digit);
      _dirty[y] = true;
    } else {
      _cache[offset] = (uint16_t)' ';
      _dirty[y] = true;
    }
  }

  if (suffix) {
    if (isOffsetInRow(offset, y)) {
      _cache[offset++] = (uint16_t)suffix;
      _dirty[y] = true;
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

  int offset = getOffset(x, y);
  for ( ; *str && isOffsetInRow(offset, y); str++, offset++) {
    _cache[offset] = (uint16_t)*str;
    _dirty[y] = true;
  }
}

void Display::log(void) 
{
  CoreMutex m(&_mutex);

  int len = (_columns + 1) * _rows;
  uint8_t *buf = new uint8_t[len];
  memset(buf, 0x00, len);
  
  int offset = 0;

  for (int y = 0; y < _rows; y++, offset++) {
    for (int x = 0; x < _columns; x++, offset++) {
      uint16_t word = _cache[getOffset(x, y)];
      buf[offset] = word < 256 ? (uint8_t)word : 0x00FF;
    }
    Log.notice("Line %d - |%s|", y, &buf[offset - _columns]);
  }
  
  delete [] buf;
}


Display *display = 0;

void init_display(void) {
  display = new Display(I2C_ADDR_SERLCD, MAX_COLUMNS, MAX_ROWS);
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