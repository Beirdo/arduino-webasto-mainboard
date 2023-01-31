#include <pico.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>

#include "project.h"
#include "ina219.h"
#include "display.h"
#include "global_timer.h"

INA219Source *ina219;
bool board_detected;
bool ignition_switch;
mutex_t startup_mutex;
mutex_t log_mutex;

void sendCRLF(Print *output, int level);
void sendCoreNum(Print *output, int level);

void setup() {
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  digitalWrite(PIN_ONBOARD_LED, HIGH);

  mutex_init(&startup_mutex);
  mutex_init(&log_mutex);

  CoreMutex m(&startup_mutex);

  pinMode(PIN_BOARD_SENSE, INPUT_PULLUP);
  delay(2);
  board_detected = !digitalRead(PIN_BOARD_SENSE);
  
  pinMode(PIN_IGNITION_SWITCH, INPUT_PULLUP);
  delay(2);
  ignition_switch = !digitalRead(PIN_IGNITION_SWITCH);

  // Make sure to shut off the pullup.
  digitalWrite(PIN_IGNITION_OUT, LOW);
  pinMode(PIN_IGNITION_OUT, OUTPUT);
  digitalWrite(PIN_IGNITION_OUT, LOW);

  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);  
  Log.setPrefix(sendCoreNum);
  Log.setSuffix(sendCRLF);

  // give user time to plugin to USB, start terminal
  delay(10000);

  Log.notice("Rebooted");
  Log.notice("Starting Core0");

  if (board_detected) {
    Log.notice("Mainboard detected");
  } else {
    Log.notice("No mainboard detected - bare Pico");
  }

  Wire.setSDA(PIN_I2C_SDA);
  Wire.setSCL(PIN_I2C_SCL);
  Wire.setClock(400000);
  Wire.begin();  

  ina219 = new INA219Source(I2C_ADDR_INA219, 12, 32, 8000);

  ina219->init();
  init_display();

  rp2040.wdt_begin(500);
}

void setup1() {
  // Give Core 0 time to initialize and take the mutex
  delay(10);

  CoreMutex m(&startup_mutex);
  Log.notice("Starting Core1");

  if (board_detected) {
    // Core 1 will be just relaying serial and running globalTimer.  Boring, but whatever
    Serial.begin(115200);
    
    Serial1.setTX(PIN_LOCAL_TX);
    Serial1.setRX(PIN_LOCAL_RX);
    Serial1.setRTS(PIN_LOCAL_RTS);
    Serial1.setCTS(PIN_LOCAL_CTS);
    Serial1.begin(115200);
  }
}

void loop() {
  static int display_count = 0;
  int topOfLoop = millis();

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
  int delayMs = clamp(100 - elapsed, 1, 100);
  delay(delayMs);

  rp2040.wdt_reset();  
}

void loop1() {
  int topOfLoop = millis();

  globalTimer.tick();

  while(Serial && Serial.available()) {
    uint8_t ch = Serial.read();
    Serial1.write(ch);
  }  

  while(Serial1 && Serial1.available()) {
    uint8_t ch = Serial1.read();
    Serial.write(ch);
  }

  int elapsed = millis() - topOfLoop;
  int delayMs = clamp(10 - elapsed, 1, 100);
  delay(delayMs);
}

void sendCoreNum(Print *output, int level)
{
  if (level > LOG_LEVEL_VERBOSE) {
    return;
  }

  mutex_enter_blocking(&log_mutex);
  int coreNum = get_core_num();
  output->print('C');  
  output->print(coreNum == 1 ? '1' : '0');
  output->print(':');
  output->print(' ');
}

void sendCRLF(Print *output, int level)
{
  if (level > LOG_LEVEL_VERBOSE) {
    return;
  }

  output->print('\n');
  output->print('\r');
  output->flush();
  mutex_exit(&log_mutex);
}
