#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "serlcd_display.h"


SerLCDDisplay::SerLCDDisplay(uint8_t i2c_address, int columns, int rows) :
  Display(i2c_address, columns, rows)
{
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

void SerLCDDisplay::setCursor(int x, int y)
{
  _lcd.setCursor(x, y);
}

void SerLCDDisplay::write(uint16_t ch)
{
  _lcd.write((uint8_t)(ch < 256 ? ch : 0x00FF));
}

void SerLCDDisplay::display(void)
{
  // Unnecessary as all changes go straight to the screen
}