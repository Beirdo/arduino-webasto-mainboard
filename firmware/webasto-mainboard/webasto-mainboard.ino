#include "analog.h"
#include "kline.h"

void setup() {
  // put your setup code here, to run once:
  init_analog();
  init_kline();

  rp2040.wdt_begin(500);
}

void loop() {
  // put your main code here, to run repeatedly:
  update_analog();
  process_kline();
  
  rp2040.wdt_reset();
}
