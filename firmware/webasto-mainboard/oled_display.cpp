#include "pico/mutex.h"
#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>
#include <Adafruit_SSD1306.h>

#include "oled_display.h"
#include "fsm.h"
#include "analog.h"
#include "fuel_pump.h"
#include "global_timer.h"

OLEDDisplay::OLEDDisplay(uint8_t i2c_address, int width, int height) :
  Display(i2c_address, width/6, height/8), _width(width), _height(height)
{
  _initialized = false;
  _ssd1306 = 0;
  if (!_connected) {
    Log.warning("No OLED connected at I2C0/%X", _i2c_address);
    return;
  }
  
  Log.notice("Found OLED at I2C0/%X", _i2c_address);

  _x_offset = (_width - (6 * _columns)) / 2;
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
  _ssd1306->setTextSize(1);
  _ssd1306->setCursor(0, 0);
  _ssd1306->cp437(true);
  _initialized = true;
}

void OLEDDisplay::setCursor(int x, int y)
{
  if (!_ssd1306 || !isConnected() || !_initialized) {
    return;
  }
  _ssd1306->setCursor(_x_offset + x * 6, y * 8);
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

  mutex_enter_blocking(&fsm_mutex);
  printLabel(0, 0, "State:");
  printHexByte(8, 0, fsm_state);

  printLabel(11, 0, "Mode:");
  printHexByte(19, 0, fsm_mode);
  mutex_exit(&fsm_mutex);

  printLabel(0, 1, "Burn Power:");
  printWatts(16, 1, fuelPumpTimer.getBurnPower());
 
  printLabel(0, 2, "Flame PTC:");
  printMilliohms(15, 2, flameDetectorSensor->get_value());

  printLabel(0, 3, "CF:");
  printPercent(6, 3, combustionFanPercent);

  printLabel(11, 3, "VF:");
  printPercent(17, 3, vehicleFanPercent);
 
  int temp = internalTempSensor->get_value();
  printLabel(0, 4, "Internal:");
  printTemperature(13, 4, temp);
 
  temp = externalTempSensor->get_value();
  printLabel(0, 5, "Outdoors:");
  printTemperature(13, 5, temp);
 
  temp = coolantTempSensor->get_value();
  printLabel(0, 6, "Coolant:");
  printTemperature(13, 6, temp);
 
  temp = exhaustTempSensor->get_value();
  printLabel(0, 7, "Exhaust:");
  printTemperature(13, 7, temp);

  update();
  if (!_connected) {
    log();
  }  
}

void oledTimerCallback(int timerId, int delayMs)
{
  Log.notice("Received OLED callback: %d, %dms", timerId, delayMs);
  OLEDDisplay *oled = dynamic_cast<OLEDDisplay *>(display);
    
  if (oled && timerId == TIMER_OLED_LOGO) {
    oled->timerCallback(timerId, delayMs);
  }
}
