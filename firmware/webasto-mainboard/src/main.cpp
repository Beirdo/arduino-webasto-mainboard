#include <Beirdo-Utilities.h>

#include <Arduino.h>
#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>
#include <cppQueue.h>
#include <canbus_mcp2517fd.h>
#include <canbus.h>

#include "project.h"
#include "wbus.h"
#include "global_timer.h"
#include "fram.h"
#include "device_eeprom.h"
#include "display.h"
#include "fsm.h"


bool mainboardDetected;
mutex_t startup_mutex;

cppQueue onboard_led_q(sizeof(bool), 4, FIFO);


void setup() 
{
  mutex_init(&startup_mutex);

  mutex_enter_blocking(&startup_mutex);

  bool ledOn = true;
  pinMode(PIN_POWER_LED, OUTPUT);
  onboard_led_q.push(&ledOn);

  pinMode(PIN_BOARD_SENSE, INPUT_PULLUP);
  delay(2);

  pinMode(PIN_I2C0_SCL, INPUT_PULLUP);
  pinMode(PIN_I2C0_SDA, INPUT_PULLUP);

  HardwareSerial *logSerial = &Serial;

  mainboardDetected = !(digitalRead(PIN_BOARD_SENSE));
  if (mainboardDetected) {
    logSerial = &Serial1;
    Serial1.setTX(PIN_SERIAL1_TX);
    Serial1.setRX(PIN_SERIAL1_RX);
  }

  logSerial->begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, logSerial);

  pinMode(PIN_CAN_SOF, INPUT);

  pinMode(PIN_OPERATING_LED, OUTPUT);
  digitalWrite(PIN_OPERATING_LED, LOW);

  pinMode(PIN_FLAME_LED, OUTPUT);
  digitalWrite(PIN_FLAME_LED, LOW);

  pinMode(PIN_CAN_EN, OUTPUT);
  digitalWrite(PIN_CAN_EN, HIGH);

  // Give user time to open a terminal to see first log messages
  delay(10000);
  Log.notice("Rebooted.");
  Log.notice("Starting Core 0");

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

  SPI.setTX(PIN_CAN_SPI_MOSI);
  SPI.setRX(PIN_CAN_SPI_MISO);
  SPI.setSCK(PIN_CAN_SPI_SCK);
  SPI.setCS(PIN_CAN_SPI_SS);

  init_canbus_mcp2517fd(&SPI, PIN_CAN_SPI_SS, PIN_CAN_INT);

  // active low enable for transceiver
  digitalWrite(PIN_CAN_EN, LOW);

  init_sensors();
  init_fram();
  init_display();
  mutex_exit(&startup_mutex);

  delay(1);

  CoreMutex m2(&startup_mutex);  
//  rp2040.wdt_begin(500);
}

void setup1(void)
{
  // Give Core0 enough time to initialize and take the mutex.
  delay(10);

  CoreMutex m(&startup_mutex);

  Log.notice("Starting Core 1");

  init_fsm();
}

void loop() {
  static int display_count = 0;

  int topOfLoop = millis();

  bool ledOn = false;
  onboard_led_q.push(&ledOn);

  display_count++;

  update_device_eeprom();
  update_fram();
  update_sensors();

  // We want screen updates every second.
  if (display_count % 10 == 1) {
    ledOn = true;
    onboard_led_q.push(&ledOn);
    update_display();
  }

  int elapsed = millis() - topOfLoop;
  if (elapsed >= 100) {
    Log.warning("Main loop > 100ms (%dms)", elapsed);
  }

  int delayMs = clamp<int>(100 - elapsed, 1, 100);

  delay(delayMs);

//  rp2040.wdt_reset();
}

void loop1(void)
{
  static bool ledOn = false;
    
  // Hi, ho!  Kermit the frog here.
  // this loop runs on core1 and is primarily just the FSM and globalTimer
  int topOfLoop = millis();

  while (!onboard_led_q.isEmpty()) {
    bool newLedOn;
    onboard_led_q.pop(&newLedOn);
    if (ledOn != newLedOn) {
      ledOn = newLedOn;
      digitalWrite(PIN_ONBOARD_LED, ledOn ? HIGH : LOW);
    }
  }

  globalTimer.tick();
  update_canbus_rx();
  update_canbus_tx();

  int elapsed = millis() - topOfLoop;
  if (elapsed >= 10) {
    Log.warning("Secondary loop > 10ms (%dms)", elapsed);
  }

  int delayMs = clamp<int>(10 - elapsed, 1, 10);
  delay(delayMs);
}
