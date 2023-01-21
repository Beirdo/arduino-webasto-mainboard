#if __cplusplus > 199211L
#define register    // Deprecated in C++11
#endif

#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>

#include "project.h"
#include "analog.h"
#include "kline.h"
#include "sensor_eeprom.h"

void setup() {
  Serial.begin(115200);
  if (Serial.available()) {
      Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  } else {
    Serial1.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial1);
  } 

  Log.notice("Starting I2C0");
  Wire.setSDA(PIN_I2C0_SDA);
  Wire.setSCL(PIN_I2C0_SCL);
  Wire.setClock(I2C0_CLK);
  Wire.begin();

  // put your setup code here, to run once:
  init_eeprom();
  init_analog();
  init_kline();

  rp2040.wdt_begin(500);
}

void loop() {
  // put your main code here, to run repeatedly:
  update_analog();
  process_kline();
  
  rp2040.wdt_reset();
}
