#include <Arduino.h>
#include <Wire.h>
#include <ArduinoLog.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "oled_display.h"
#include "project.h"

Display *display = 0;

Display::Display(uint8_t i2c_address, int columns, int rows) : 
  _i2c_address(i2c_address), _columns(columns), _rows(rows) 
{
  Log.notice("Attempting to connect to %dx%d display at I2C %X", _columns, _rows, _i2c_address);
  
  isConnected();  

  int len = _columns * _rows;
  _cache = new uint16_t[len];
  _display = new uint16_t[len];
  _dirty = new bool[_rows];

  for (int i = 0; i < _rows; i++ ) {
    clearLine(i);
    clearMirrorLine(i);
    _dirty[i] = true;
  }
}

Display::~Display(void)
{
  delete [] _cache;
  delete [] _display;
  delete [] _dirty;
}

volatile bool Display::isConnected(void)
{
  Wire.begin();
  Wire.beginTransmission(_i2c_address);
  int ret = Wire.endTransmission();
  _connected = !ret;
  // Log.notice("I2C Probe of %X - %d -> %d", _i2c_address, ret, _connected);
  return (_connected);
}

void Display::update(void)
{
  if (!isConnected() || !_dirty) {
    return;
  }

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
          setCursor(x, y);
          cursorX = x;
          cursorY = y;
        }

        write(_cache[offset]);
        _display[offset] = _cache[offset];
        cursorX++;
      }
    }

    _dirty[y] = false;    
  }
  display();
}

void Display::clearLine(int y)
{
  uint16_t *buf = &_cache[getOffset(0, y)];

  for (int i = 0; i < _columns; i++) {
    *(buf++) = 0x0020;
  }
  _dirty[y] = true;
}


void Display::clearMirrorLine(int y)
{
  uint16_t *buf = &_display[getOffset(0, y)];

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

void Display::printHexByte(int x, int y, uint8_t data)
{
  if (y < 0 || y >= _rows || x < 0) {
    return;
  }

  int offset = getOffset(x, y);
  if (isOffsetInRow(offset, y)) {
    _cache[offset++] = getHexDigit(HI_NIBBLE(data));
    _dirty[y] = true;
  }

  if (isOffsetInRow(offset, y)) {
    _cache[offset++] = getHexDigit(LO_NIBBLE(data));
    _dirty[y] = true;
  }
}

void Display::printDigits(int x, int y, int value, int count, uint8_t suffix, bool nonZero)
{
  if (y < 0 || y >= _rows || x < 0 || count < 0 || count > 6) {
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

void Display::printWatts(int x, int y, int power)
{
  printDigits(x, y, power, 4, 'W');
}

void Display::printPercent(int x, int y, int percent)
{
  printDigits(x, y, percent, 3, '%');
}

void Display::printTemperature(int x, int y, int temperature)
{
  printDigits(x, y, temperature / 100, 4, '.');
  printDigits(x + 5, y, abs(temperature) % 100, 2, 'C', true);
}

void Display::printMilliohms(int x, int y, int milliohms)
{
  printDigits(x, y, milliohms, 5, 'm');
}
    

void Display::printLabel(int x, int y, const char *str)
{
  if (y < 0 || y >= _rows || x < 0 || !str) {
    return;
  }

  int offset = getOffset(x, y);
  for ( ; *str && isOffsetInRow(offset, y); str++, offset++) {
    _cache[offset] = (uint16_t)*str;
    _dirty[y] = true;
  }
}

void Display::log(void) 
{
  if (_connected) {
    return;
  }

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

void init_display(void) {
  if (display) {
    Log.error("WTF");
    return;    
  }

  display = 0;
  display = new OLEDDisplay(I2C_ADDR_OLED, 128, 64);
  if (!display || !display->isConnected()) {
    if (display) {
      delete display;
    }
  }
}

void update_display(void) {
  if (!display) {
    init_display();
  }

  if (display) {
    display->updateDisplay();
  }

  if (!display || !display->isConnected()) {
    if (display) {
      delete display;
      display = 0;
    }
  }
}

