#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>
#include <Adafruit_SSD1306.h>

#include "oled_display.h"


OLEDDisplay::OLEDDisplay(uint8_t i2c_address, int width, int height) :
  Display(i2c_address, width/6, height/8), _width(width), _height(height)
{
  _ssd1306 = 0;
  if (!_connected) {
    Log.warning("No OLED connected at I2C0/0x%X", _i2c_address);
    return;
  }
  
  Log.notice("Found OLED at I2C0/0x%X", _i2c_address);

  _x_offset = (_width - (6 * _columns)) / 2;
  _ssd1306 = new Adafruit_SSD1306(_width, _height, &Wire, -1);

  int len = _width * _height / 8;
  _ssd1306->attachRAM(0, 0, len);
  _ssd1306->begin(SSD1306_SWITCHCAPVCC, _i2c_address);

  _ssd1306->display();

  delay(2000);
  
  _ssd1306->clearDisplay();
  _ssd1306->setTextColor(SSD1306_WHITE);
  _ssd1306->setTextSize(1);
  _ssd1306->setCursor(0, 0);
  _ssd1306->cp437(true);
}

void OLEDDisplay::setCursor(int x, int y)
{
  if (!_ssd1306 || !_connected) {
    return;
  }
  _ssd1306->setCursor(_x_offset + x * 6, y * 8);
}

void OLEDDisplay::write(uint16_t ch)
{
  if (!_ssd1306 || !_connected) {
    return;
  }
  _ssd1306->write((uint8_t)(ch < 256 ? ch : 0x00FF));
}

void OLEDDisplay::display(void)
{
  if (!_ssd1306 || !_connected) {
    return;
  }
  _ssd1306->display();
}
