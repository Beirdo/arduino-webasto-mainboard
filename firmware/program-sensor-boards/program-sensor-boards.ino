#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>

#include "project.h"
#include "sensor_eeprom.h"

void sendCoreNum(Print *output, int level);
void sendCRLF(Print *output, int level);

mutex_t log_mutex;

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

  mutex_init(&log_mutex);
  Log.setPrefix(sendCoreNum);
  Log.setSuffix(sendCRLF);
  delay(10000);
  Log.notice("Rebooted.");

  Log.notice("Starting I2C0");
  Wire.setSDA(PIN_I2C0_SDA);
  Wire.setSCL(PIN_I2C0_SCL);
  Wire.setClock(I2C0_CLK);
  Wire.begin();

  init_eeprom();
}

void loop() {
  for (;;) {
    delay(1000);
  }
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

