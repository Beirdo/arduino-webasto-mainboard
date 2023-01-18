#include <pico.h>
#include <Wire.h>

#include "analog.h"
#include "ds2482.h"
#include "ads7823.h"
#include "mcp96l01.h"
#include "internal_adc.h"
#include "ina219.h"

// goes into glow_plug later
#define GLOW_PLUG_IN_EN_PIN 7
#define GLOW_PLUG_OUT_EN_PIN 6
volatile bool glow_plug_in_enable;
volatile bool glow_plug_out_enable;

DS2482Source externalTempSensor(0x18, 9);
ADS7823Source batteryVoltageSensor(0x48, 12, 19767, 4096);
MCP96L01Source coolantTempSensor(0x60, 16, TYPE_K, 4);
MCP96L01Source exhaustTempSensor(0x67, 16, TYPE_K, 4);
InternalADCSource internalTempSensor(4, 12);
INA219Source flameDetectorSensor(0x4F, 12, &glow_plug_in_enable);

void set_open_drain_pin(int pinNum, int value)
{
  if (value) {
    pinMode(pinNum, INPUT);
    digitalWrite(pinNum, LOW);  // shutoff builtin pullup
  } else {
    pinMode(pinNum, OUTPUT);
    digitalWrite(pinNum, LOW);
  }
}

void init_analog(void)
{
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.begin();
  Wire.setClock(400000);

  glow_plug_in_enable = false;
  set_open_drain_pin(GLOW_PLUG_IN_EN_PIN, glow_plug_in_enable);

  glow_plug_out_enable = false;
  set_open_drain_pin(GLOW_PLUG_OUT_EN_PIN, glow_plug_out_enable);

  externalTempSensor.init();
  batteryVoltageSensor.init();
  coolantTempSensor.init();
  exhaustTempSensor.init();
  internalTempSensor.init();
  flameDetectorSensor.init();
}

void update_analog(void) 
{
  externalTempSensor.update();
  batteryVoltageSensor.update();
  coolantTempSensor.update();
  exhaustTempSensor.update();
  internalTempSensor.update();
  flameDetectorSensor.update();
}
