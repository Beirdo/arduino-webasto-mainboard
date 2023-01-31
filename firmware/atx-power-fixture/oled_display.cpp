#include "pico/mutex.h"
#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>
#include <Adafruit_SSD1306.h>

#include "oled_display.h"
#include "global_timer.h"
#include "ina219.h"

extern INA219Source *ina219;

OLEDDisplay::OLEDDisplay(uint8_t i2c_address, int width, int height) :
  Display(i2c_address, width/11, height/16), _width(width), _height(height)
{
  _initialized = false;
  _ssd1306 = 0;
  if (!_connected) {
    Log.warning("No OLED connected at I2C0/%X", _i2c_address);
    return;
  }
  
  Log.notice("Found OLED at I2C0/%X", _i2c_address);

  _x_offset = (_width - (11 * _columns)) / 2;
  _y_offset = (_height - (16 * _rows)) / 2;  
  _ssd1306 = new Adafruit_SSD1306(_width, _height, &Wire, -1);

  int len = _width * _height / 8;
  _ssd1306->attachRAM(0, 0, len);
  _ssd1306->begin(SSD1306_SWITCHCAPVCC, _i2c_address);

  _ssd1306->display();

  globalTimer.register_timer(TIMER_OLED_LOGO, 2000, &oledTimerCallback);
}

OLEDDisplay::~OLEDDisplay(void)
{
  delete _ssd1306;
}

void OLEDDisplay::timerCallback(int timerId, int delayMs)
{
  Log.notice("OLED timer: delay %dms", delayMs);
  if (timerId != TIMER_OLED_LOGO) {
    return;
  }

  _ssd1306->clearDisplay();
  _ssd1306->setTextColor(SSD1306_WHITE);
  _ssd1306->setTextSize(2);
  _ssd1306->setCursor(0, 0);
  _ssd1306->cp437(true);
  _initialized = true;
}

void OLEDDisplay::setCursor(int x, int y)
{
  if (!_ssd1306 || !isConnected() || !_initialized) {
    return;
  }
  _ssd1306->setCursor(_x_offset + x * 11, _y_offset + y * 16);
}

void OLEDDisplay::write(uint16_t ch)
{
  if (!_ssd1306 || !isConnected() || !_initialized) {
    return;
  }
  _ssd1306->write((uint8_t)(ch < 256 ? ch : 0x00FF));
}

void OLEDDisplay::clearDisplay(void)
{
  if (!_ssd1306 || !isConnected() || !_initialized) {
    return;
  }
  _ssd1306->clearDisplay();
}

void OLEDDisplay::display(void)
{
  if (!_ssd1306 || !isConnected() || !_initialized) {
    return;
  }
  _ssd1306->display();
}

void OLEDDisplay::updateDisplay(void)
{  
  clearDisplay();

  for (int y = 0; y < _rows; y++) {
    clearLine(y);
    clearMirrorLine(y);
  }

  printLabel(0, 0, "Vs:");
  printDigits(4, 0, ina219->get_value(0), 5);
  printLabel(9, 0, "mV");  

  printLabel(0, 1, "Vb:");
  printDigits(4, 1, ina219->get_value(1), 5);
  printLabel(9, 1, "mV");  
 
  printLabel(0, 2, "Pb:");
  printDigits(4, 2, ina219->get_value(2), 5);
  printLabel(9, 2, "mW");  

  printLabel(0, 3, "Ib:");
  printDigits(4, 3, ina219->get_value(2), 5);
  printLabel(9, 3, "mA");

  update();
}

void oledTimerCallback(int timerId, int delayMs)
{
  Log.notice("Received OLED callback: %d, %dms", timerId, delayMs);
  OLEDDisplay *oled = dynamic_cast<OLEDDisplay *>(display);
    
  if (oled && timerId == TIMER_OLED_LOGO) {
    oled->timerCallback(timerId, delayMs);
  }
}
