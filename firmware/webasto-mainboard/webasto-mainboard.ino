#include "analog.h"
#include "kline.h"

void setup() {
  glow_plug_in_enable = false;

  // put your setup code here, to run once:
  init_analog();
  init_kline();

  rp2040.wdt_begin(200);
}

void loop() {
  // put your main code here, to run repeatedly:
  update_analog();
  process_kline();
  
  rp2040.wdt_reset();
}
