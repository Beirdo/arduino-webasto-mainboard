#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>
#include <cppQueue.h>
#include <TCA9534-GPIO.h>
#include <Beirdo-Utilities.h>

#include "project.h"
#include "analog.h"
#include "wbus.h"
#include "sensor_eeprom.h"
#include "global_timer.h"
#include "fram.h"
#include "device_eeprom.h"
#include "display.h"
#include "fsm.h"
#include "cbor.h"
#include "canbus.h"

bool mainboardDetected;
mutex_t startup_mutex;

cppQueue onboard_led_q(sizeof(bool), 4, FIFO);
TCA9534 tca9534;


void setup() {
  mutex_init(&startup_mutex);

  {
    CoreMutex m(&startup_mutex);

    bool ledOn = true;
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

    tca9534.begin(Wire, I2C_ADDR_TCA9534);
    tca9534.pinMode(PIN_CAN_SOF, INPUT);

    tca9534.pinMode(PIN_POWER_LED, OUTPUT);
    tca9534.digitalWrite(PIN_POWER_LED, HIGH);

    tca9534.pinMode(PIN_OPERATING_LED, OUTPUT);
    tca9534.digitalWrite(PIN_OPERATING_LED, LOW);

    tca9534.pinMode(PIN_FLAME_LED, OUTPUT);
    tca9534.digitalWrite(PIN_FLAME_LED, LOW);

    tca9534.pinMode(PIN_CAN_EN, OUTPUT);
    tca9534.digitalWrite(PIN_CAN_EN, HIGH);

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

    init_cbor();
    init_canbus();
    init_eeprom();
    init_fram();
    init_analog();
    init_display();
  }

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
  init_wbus();
}

void loop() {
  static int display_count = 0;

  int topOfLoop = millis();

  bool ledOn = false;
  onboard_led_q.push(&ledOn);

  display_count++;

  update_device_eeprom();
  update_fram();
  update_analog();

  // We want screen updates every second.
  if (display_count % 10 == 1) {
    ledOn = true;
    onboard_led_q.push(&ledOn);
    update_display();
    cbor_send();
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
      // Note:  do NOT use the LED connected to the WiFi chip.  Using it messes with the WiFi's use of
      //        the shared SPI bus.  Hilarity (well, crashes) ensue
      tca9534.digitalWrite(PIN_POWER_LED, ledOn ? HIGH : LOW);
    }
  }

  globalTimer.tick();
  update_canbus_rx();
  update_canbus_tx();
  receive_wbus_from_serial();
  process_wbus();

  int elapsed = millis() - topOfLoop;
  if (elapsed >= 10) {
    Log.warning("Secondary loop > 10ms (%dms)", elapsed);
  }

  int delayMs = clamp<int>(10 - elapsed, 1, 10);
  delay(delayMs);
}
