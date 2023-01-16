#define PIN_SERIAL1_TX  4
#define PIN_SERIAL1_RX  5
#define PIN_KLINE_EN    3

#include <Serial.h>

#include "kline.h"

void init_kline(void)
{
  Serial1.setRX(PIN_SERIAL1_RX);
  Serial1.setTX(PIN_SERIAL1_TX);
  Serial1.begin(2400, SERIAL_8E1);

  pinMode(PIN_KLINE_EN, OUTPUT);
  digitalWrite(PIN_KLINE_EN, LOW);  
}

void process_kline(void)
{
  
}
