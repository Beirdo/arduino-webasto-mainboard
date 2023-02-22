#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>
#include <cppQueue.h>

#include "project.h"
#include "analog.h"
#include "wbus.h"
#include "sensor_eeprom.h"
#include "global_timer.h"
#include "fram.h"
#include "device_eeprom.h"
#include "display.h"
#include "fsm.h"
#include "wifi.h"

bool mainboardDetected;
mutex_t startup_mutex;
mutex_t log_mutex;

cppQueue onboard_led_q(sizeof(bool), 4, FIFO);

void sendCRLF(Print *output, int level);
void sendCoreNum(Print *output, int level);


void setup() {
  mutex_init(&startup_mutex);
  mutex_init(&log_mutex);

  {
    CoreMutex m(&startup_mutex);

    bool ledOn = true;
    onboard_led_q.push(&ledOn);

    pinMode(PIN_BOARD_SENSE, INPUT_PULLUP);
    delay(2);

    pinMode(PIN_I2C0_SCL, INPUT_PULLUP);
    pinMode(PIN_I2C0_SDA, INPUT_PULLUP);

    mainboardDetected = !(digitalRead(PIN_BOARD_SENSE));
    if (!mainboardDetected) {
      Serial.begin(115200);
      Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    } else {
      Serial1.setTX(PIN_SERIAL1_TX);
      Serial1.setRX(PIN_SERIAL1_RX);
      Serial1.setCTS(PIN_SERIAL1_CTS);
      Serial1.setRTS(PIN_SERIAL1_RTS);
      Serial1.begin(115200);
      Log.begin(LOG_LEVEL_VERBOSE, &Serial1);
    }

    Log.setPrefix(sendCoreNum);
    Log.setSuffix(sendCRLF);

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

  pinMode(PIN_ONBOARD_LED, OUTPUT);

  init_wifi();
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
  // Hi, ho!  Kermit the frog here.
  // this loop runs on core1 and is primarily just the FSM and globalTimer
  int topOfLoop = millis();

  while (!onboard_led_q.isEmpty()) {
    bool ledOn;
    onboard_led_q.pop(&ledOn);
    digitalWrite(PIN_ONBOARD_LED, ledOn ? HIGH : LOW);
  }

  globalTimer.tick();
  update_wifi(); 
  receive_wbus_from_serial();
  process_wbus();

  int elapsed = millis() - topOfLoop;
  if (elapsed >= 10) {
    Log.warning("Secondary loop > 10ms (%dms)", elapsed);
  }

  int delayMs = clamp<int>(10 - elapsed, 1, 10);
  delay(delayMs);
}

void sendCoreNum(Print *output, int level)
{
  if (level > LOG_LEVEL_VERBOSE) {
    return;
  }

  mutex_enter_blocking(&log_mutex);
  int coreNum = get_core_num();
  output->printf("C%d: %20d: ", coreNum, millis());
}

void sendCRLF(Print *output, int level)
{
  if (level > LOG_LEVEL_VERBOSE) {
    return;
  }

  output->printf("\n\r");
  output->flush();
  mutex_exit(&log_mutex);
}

void hexdump(const void* mem, uint32_t len, uint8_t cols) 
{
  const char* src = (const char*)mem;
  static char line[128];
  char *ch = line;
  int written;  

  memset(ch, 0x00, 128);
      
  written = sprintf(ch, "[HEXDUMP] Address: %08X len: %04X (%d)", src, len, len);
  Log.notice("%s", line);

  while (len > 0) {
    ch = line;
    memset(ch, 0x00, 128);

    uint32_t linesize = cols > len ? len : cols;
    written = sprintf(ch, "[%08X] 0x%04X: ", src, (int)(src - (const char*)mem));
    ch += written;
    for (uint32_t i = 0; i < linesize; i++) {
        written = sprintf(ch, "%02X ", *(src + i));
        ch += written;
    }
    written = sprintf(ch, "  ");
    ch += written;

    for (uint32_t i = linesize; i < cols; i++) {
        written = sprintf(ch, "   ");
        ch += written;
    }

    for (uint32_t i = 0; i < linesize; i++) {
        unsigned char c = *(src + i);
        *(ch++) = isprint(c) ? c : '.';
    }

    src += linesize;
    len -= linesize;
    Log.info("%s", line);
  }
}

