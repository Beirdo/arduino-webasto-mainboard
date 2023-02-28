#include <Arduino.h>
#include <ArduinoLog.h>
#include <sensor.h>
#include <Beirdo-Utilities.h>
#include <canbus_stm32.h>
#include <canbus_ids.h>
#include <canbus.h>
#include <Wire.h>

#include "project.h"
#include "ds2482.h"

DS2482Sensor externalTempSensor(CANBUS_ID_EXTERNAL_TEMP, I2C_ADDRESS_DS2482, 12);

CAN_filter_t filters[] = {
  {0, 0, 0, 0, 
   0xFF00 | STM32_CAN::makeFilter16(CANBUS_ID_EXTERNAL_TEMP, STANDARD_FORMAT, REMOTE_FRAME),
   0xFF00 | STM32_CAN::makeFilter16(CANBUS_ID_EXTERNAL_TEMP | CANBUS_ID_WRITE_MODIFIER, STANDARD_FORMAT, DATA_FRAME)},  
};
int filter_count = NELEMS(filters);

void setup(void)
{
  Serial.setRx(PIN_USART2_RX);
  Serial.setTx(PIN_USART2_TX);
  Serial.begin(115200);
  setup_logging(LOG_LEVEL_VERBOSE, &Serial);

  Wire.begin();
  Wire.setClock(400000);

  externalTempSensor.init();

  init_canbus_stm32_internal(PIN_CAN_EN, filters, filter_count);
}

void loop(void)
{
  int topOfLoop = millis();
  
  externalTempSensor.update();

  update_canbus_rx();
  update_canbus_tx();

  int duration = millis() - topOfLoop;
  if (duration >= 100) {
    Log.warning("Loop duration (%dms) > 100ms", duration);
  }

  int delay_ms = clamp<int>(100 - duration, 1, 100);

  delay(delay_ms);
}
