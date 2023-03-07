#include <Arduino.h>
#include <ArduinoLog.h>
#include <Beirdo-Utilities.h>
#include <Wire.h>

#include "project.h"
#include "ina219.h"
#include "display.h"
#include "global_timer.h"

INA219Source *ina219;
bool ignition_switch;

void setup() {
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  digitalWrite(PIN_ONBOARD_LED, HIGH);
  
  pinMode(PIN_IGNITION_SWITCH, INPUT_PULLUP);
  delay(2);
  ignition_switch = !digitalRead(PIN_IGNITION_SWITCH);

  // Make sure to shut off the pullup.
  digitalWrite(PIN_IGNITION_OUT, LOW);
  pinMode(PIN_IGNITION_OUT, OUTPUT);
  digitalWrite(PIN_IGNITION_OUT, LOW);

  Serial.begin(115200);
  setup_logging(LOG_LEVEL_VERBOSE, &Serial);  

  Log.notice("Rebooted");

  Wire.setClock(400000);
  Wire.begin();  

  ina219 = new INA219Source(I2C_ADDR_INA219, 12, 32, 8000);

  ina219->init();
  init_display();
}

void loop() {
  static int display_count = 0;
  int topOfLoop = millis();

  globalTimer.tick();

  digitalWrite(PIN_ONBOARD_LED, LOW);
  display_count++;

  ina219->update();

  ignition_switch = !digitalRead(PIN_IGNITION_SWITCH);
  digitalWrite(PIN_IGNITION_OUT, ignition_switch);

  // update screen
  if (display_count % 10 == 1) {
    digitalWrite(PIN_ONBOARD_LED, HIGH);
    update_display();
  }

  int elapsed = millis() - topOfLoop;
  int delayMs = clamp<int>(100 - elapsed, 1, 100);
  delay(delayMs);
}
