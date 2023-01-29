#include "pico/mutex.h"
#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>

#include "serlcd_display.h"
#include "fsm.h"
#include "analog.h"
#include "fuel_pump.h"


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
  // Unnecessary as all changes go straight to the screen on update
}

void SerLCDDisplay::clearDisplay(void)
{
  // Unnecessary as all changes go straight to the screen on update
}

void SerLCDDisplay::updateDisplay(void)
{
  mutex_enter_blocking(&fsm_mutex);
  int state = fsm_state;
  printLabel(0, 0, "State:");
  printHexByte(7, 0, state);
 
  printLabel(10, 0, "P:");
  printWatts(13, 0, fuelPumpTimer.getBurnPower());
  mutex_exit(&fsm_mutex);

  printLabel(0, 1, "Fl:");
  printMilliohms(4, 1, flameDetectorSensor->get_value());
 
  printLabel(10, 1, "Fan:");
  printPercent(15, 1, combustionFanPercent);
 
  int temp = internalTempSensor->get_value();
  printLabel(0, 2, "I:");
  printTemperature(2, 2, temp);
 
  temp = externalTempSensor->get_value();
  printLabel(10, 2, "O:");
  printTemperature(12, 2, temp);
 
  temp = coolantTempSensor->get_value();
  printLabel(0, 3, "C:");
  printTemperature(2, 3, temp);
 
  temp = exhaustTempSensor->get_value();
  printLabel(10, 3, "E:");
  printTemperature(12, 3, temp);
 
  update();
  log();
}