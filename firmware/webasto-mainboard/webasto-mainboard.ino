#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>

#include "project.h"
#include "analog.h"
#include "kline.h"
#include "sensor_eeprom.h"
#include "global_timer.h"
#include "fram.h"
#include "device_eeprom.h"
#include "display.h"
#include "fsm.h"

bool mainboardDetected;
mutex_t startup_mutex;

void sendCRLF(Print *output, int level);

void setup() {
  mutex_init(&startup_mutex);

  CoreMutex m(&startup_mutex);
  
  pinMode(PIN_ONBOARD_LED, OUTPUT);

  pinMode(PIN_USE_USB, INPUT_PULLUP);
  pinMode(PIN_BOARD_SENSE, INPUT_PULLUP);
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
  delay(10000);
  Log.notice("Rebooted.");
  Log.notice("Setting up Core 0");

  mainboardDetected = !(digitalRead(PIN_BOARD_SENSE));
  if (mainboardDetected) {
    Log.notice("Mainboard detected");
  } else {
    Log.notice("No mainboard detected - bare Pico");
  }

  delay(500);

  init_device_eeprom();
  
  Log.notice("Starting I2C0");
  Wire.setSDA(PIN_I2C0_SDA);
  Wire.setSCL(PIN_I2C0_SCL);
  Wire.setClock(I2C0_CLK);
  Wire.begin();

  // put your setup code here, to run once:
  init_eeprom();
  init_fram();
  init_analog();
  //init_kline();
  init_display();

  // rp2040.wdt_begin(500);
}

void setup1(void) 
{
  // Give Core0 enough time to initialize and take the mutex.
  delay(10);

  CoreMutex m(&startup_mutex);

  Log.notice("Starting Core1");
  init_fsm();
}

void loop() {
  delay(1000);
  int topOfLoop = millis();

  // put your main code here, to run repeatedly:
  update_device_eeprom();
  update_fram();
  update_analog();
  //process_kline();
  update_display();

  static bool led = true;
  led = !led;
  digitalWrite(PIN_ONBOARD_LED, led);  
  
  int elapsed = millis() - topOfLoop;
  if (elapsed >= 100) {
    Log.warning("Main loop > 100ms (%dms)", elapsed);
  }

  int delayMs = clamp(100 - elapsed, 1, 100);
  delay(delayMs);
  // rp2040.wdt_reset();
}

void loop1(void)
{
  // Hi, ho!  Kermit the frog here.
  // this loop runs on core1 and is primarily just the FSM and globalTimer 
  int topOfLoop = millis();

  globalTimer.tick();

  int elapsed = millis() - topOfLoop;
  if (elapsed >= 10) {
    Log.warning("Secondary loop > 10ms (%dms)", elapsed);
  }

  int delayMs = clamp(10 - elapsed, 1, 10);
  delay(delayMs);
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
