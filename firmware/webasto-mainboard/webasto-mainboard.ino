#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>

#include "project.h"
#include "analog.h"
#include "kline.h"
#include "sensor_eeprom.h"
#include "global_timer.h"
#include "fram.h"
#include "device_eeprom.h"

void sendCRLF(Print *output, int level);

void setup() {
  pinMode(PIN_ONBOARD_LED, OUTPUT);

  pinMode(PIN_USE_USB, INPUT_PULLUP);
  delay(2);

  if (digitalRead(PIN_USE_USB)) {
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    digitalWrite(PIN_ONBOARD_LED, HIGH);
  } else {
    Serial1.setTX(PIN_SERIAL1_TX);
    Serial1.setRX(PIN_SERIAL1_RX);
    Serial1.setCTS(PIN_SERIAL1_CTS);
    Serial1.setRTS(PIN_SERIAL1_RTS);
    Serial1.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial1);
    digitalWrite(PIN_ONBOARD_LED, LOW);
  }

  Log.setSuffix(sendCRLF);
  delay(1000);
  Log.notice("Rebooted.");

  init_device_eeprom();
  
  Log.notice("Starting I2C0");
  Wire.setSDA(PIN_I2C0_SDA);
  Wire.setSCL(PIN_I2C0_SCL);
  Wire.setClock(I2C0_CLK);
  Wire.begin();

  // put your setup code here, to run once:
  init_eeprom();
  //init_fram();
  //init_analog();
  //init_kline();

  rp2040.wdt_begin(500);
}

void loop() {
  int topOfLoop = millis();

  // put your main code here, to run repeatedly:
  update_device_eeprom();
  //update_fram();
  //update_analog();
  //process_kline();
  //globalTimer.tick();

  static bool led = true;
  led = !led;
  digitalWrite(PIN_ONBOARD_LED, led);  
  
  int elapsed = millis() - topOfLoop;
  if (elapsed >= 100) {
    Log.warning("Main loop > 100ms (%dms)", elapsed);
  }

  int delayMs = clamp(100 - elapsed, 1, 100);
  delay(delayMs);
  rp2040.wdt_reset();
}

void sendCRLF(Print *output, int level)
{
  if (level > LOG_LEVEL_VERBOSE) {
    return;
  }
  output->print('\n');
  output->print('\r');
  output->flush();
}
