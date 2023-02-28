#include <Arduino.h>
#include <ArduinoLog.h>
#include <sensor.h>
#include <Beirdo-Utilities.h>
#include <canbus_stm32.h>
#include <canbus_ids.h>
#include <canbus.h>

#include "project.h"
#include "internal_gpio.h"

InternalGPIOSensor startRunSensor(CANBUS_ID_START_RUN, PIN_START_RUN);
InternalGPIOSensor ignitionSenseSensor(CANBUS_ID_IGNITION_SENSE, PIN_IGNITION_SENSE);
InternalGPIOSensor emergencyStopSensor(CANBUS_ID_EMERGENCY_STOP, PIN_EMERGENCY_STOP);

CAN_filter_t filters[] = {
  {0, 0, 0, 0, 
   0xFF00 | STM32_CAN::makeFilter16(CANBUS_ID_START_RUN, STANDARD_FORMAT, REMOTE_FRAME),
   0xFF00 | STM32_CAN::makeFilter16(CANBUS_ID_START_RUN | CANBUS_ID_WRITE_MODIFIER, STANDARD_FORMAT, DATA_FRAME)},  
  {1, 0, 0, 1, 
   0xFF00 | STM32_CAN::makeFilter16(CANBUS_ID_IGNITION_SENSE, STANDARD_FORMAT, REMOTE_FRAME),
   0xFF00 | STM32_CAN::makeFilter16(CANBUS_ID_IGNITION_SENSE | CANBUS_ID_WRITE_MODIFIER, STANDARD_FORMAT, DATA_FRAME)},  
  {2, 0, 0, 1, 
   0xFF00 | STM32_CAN::makeFilter16(CANBUS_ID_EMERGENCY_STOP, STANDARD_FORMAT, REMOTE_FRAME),
   0xFF00 | STM32_CAN::makeFilter16(CANBUS_ID_EMERGENCY_STOP | CANBUS_ID_WRITE_MODIFIER, STANDARD_FORMAT, DATA_FRAME)},  
};
int filter_count = NELEMS(filters);

void setup(void)
{
  Serial.setRx(PIN_USART2_RX);
  Serial.setTx(PIN_USART2_TX);
  Serial.begin(115200);
  setup_logging(LOG_LEVEL_VERBOSE, &Serial);

  startRunSensor.init();
  ignitionSenseSensor.init();
  emergencyStopSensor.init();

  init_canbus_stm32_internal(PIN_CAN_EN, filters, filter_count);
}

void loop(void)
{
  int topOfLoop = millis();
  
  startRunSensor.update();
  ignitionSenseSensor.update();
  emergencyStopSensor.update();

  update_canbus_rx();
  update_canbus_tx();

  int duration = millis() - topOfLoop;
  if (duration >= 100) {
    Log.warning("Loop duration (%dms) > 100ms", duration);
  }

  int delay_ms = clamp<int>(100 - duration, 1, 100);

  delay(delay_ms);
}
