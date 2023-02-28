#include <Arduino.h>
#include <ArduinoLog.h>
#include <sensor.h>
#include <Beirdo-Utilities.h>
#include <canbus_stm32.h>
#include <canbus_ids.h>
#include <canbus.h>

#include "project.h"
#include "internal_adc.h"
#include "ntc_thermistor.h"

int32_t ntc_voltage_to_temperature(int32_t voltage);

InternalADCSensor vrefSensor(0, 17, 12);
InternalADCSensor coolantTempSensor(CANBUS_ID_COOLANT_TEMP_WEBASTO, 0, 12, 0, 0, ntc_voltage_to_temperature);
InternalADCSensor batteryVoltageSensor(CANBUS_ID_BATTERY_VOLTAGE, 1, 12, 599, 100);

int32_t vref = -1;

void setup(void)
{
  Serial.setRx(PIN_USART2_RX);
  Serial.setTx(PIN_USART2_TX);
  Serial.begin(115200);
  setup_logging(LOG_LEVEL_VERBOSE, &Serial);

  vrefSensor.init();
  batteryVoltageSensor.init();
  coolantTempSensor.init();

  init_canbus_stm32_internal(PIN_CAN_EN);
}

void loop(void)
{
  int topOfLoop = millis();
  
  vrefSensor.update();
  vref = vrefSensor.get_value();
  
  batteryVoltageSensor.update();
  coolantTempSensor.update();

  update_canbus_rx();
  update_canbus_tx();

  int duration = millis() - topOfLoop;
  if (duration >= 100) {
    Log.warning("Loop duration (%dms) > 100ms", duration);
  }

  int delay_ms = clamp<int>(100 - duration, 1, 100);

  delay(delay_ms);
}

int32_t ntc_voltage_to_temperature(int32_t voltage)
{
  if (vref == UNUSED_VALUE || voltage == UNUSED_VALUE || voltage >= vref) {
    return UNUSED_VALUE;
  }

  // Thermistor resistance can be calculated via a voltage divider with a 10k pull-up, and thermistor to ground
  // Vout / Vin = Rth / (Rth + 10k)
  // Vout * (Rth + 10k) = Vin * Rth
  // Vout * 10k + Vout * Rth = Vin * Rth
  // Vin * Rth - (Vout * Rth) = Vout * 10k
  // Rth (Vin - Vout) = Vout * 10k
  // Rth = 10k * Vout / (Vin - Vout)
  int32_t resistance = 10000 * voltage / (vref - voltage);
  int32_t temperature = thermistor.lookup(resistance);

  return temperature;
}
