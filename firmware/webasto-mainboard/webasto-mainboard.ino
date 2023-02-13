#include <pico.h>
#include <Wire.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>

#include "project.h"
#include "analog.h"
#include "kline.h"
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

void sendCRLF(Print *output, int level);
void sendCoreNum(Print *output, int level);


void setup() {
  mutex_init(&startup_mutex);
  mutex_init(&log_mutex);

  CoreMutex m(&startup_mutex);
  
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  digitalWrite(PIN_ONBOARD_LED, HIGH);

  pinMode(PIN_USE_USB, INPUT_PULLUP);
  pinMode(PIN_BOARD_SENSE, INPUT_PULLUP);
  delay(2);

  pinMode(PIN_I2C0_SCL, INPUT_PULLUP);
  pinMode(PIN_I2C0_SDA, INPUT_PULLUP);

  if (digitalRead(PIN_USE_USB) && 0) {
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

  mainboardDetected = !(digitalRead(PIN_BOARD_SENSE));
  if (mainboardDetected) {
    Log.notice("Mainboard detected");
  } else {
    Log.notice("No mainboard detected - bare Pico");
  }

  delay(500);

  init_device_eeprom();

  init_wifi();
  
  Log.notice("Starting I2C0");
  Wire.setSDA(PIN_I2C0_SDA);
  Wire.setSCL(PIN_I2C0_SCL);
  Wire.setClock(I2C0_CLK);
  Wire.begin();

  init_eeprom();
  init_fram();
  init_analog();
  delay(100);
  init_display();

  rp2040.wdt_begin(500);
}

void setup1(void) 
{
  // Give Core0 enough time to initialize and take the mutex.
  delay(10);

  CoreMutex m(&startup_mutex);

  Log.notice("Starting Core 1");
  init_fsm();
  init_kline();
}

void loop() {
  static int display_count = 0;

  int topOfLoop = millis();

  digitalWrite(PIN_ONBOARD_LED, LOW);  

  display_count++;

  update_device_eeprom();
  update_fram();
  update_analog();

  // We want screen updates every second.
  if (display_count % 10 == 1) {
    digitalWrite(PIN_ONBOARD_LED, HIGH);  
    update_display();
    cbor_send();
  }

  update_wifi();

  int elapsed = millis() - topOfLoop;
  if (elapsed >= 100) {
    Log.warning("Main loop > 100ms (%dms)", elapsed);
  }

  int delayMs = clamp<int>(100 - elapsed, 1, 100);
  delay(delayMs);
  
  rp2040.wdt_reset();
}

void loop1(void)
{
  // Hi, ho!  Kermit the frog here.
  // this loop runs on core1 and is primarily just the FSM and globalTimer 
  int topOfLoop = millis();

  globalTimer.tick();
  receive_kline_from_serial();
  process_kline();

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

void hexdump(const void* mem, uint32_t len, uint8_t cols) {
    const char* src = (const char*)mem;
    printf("\n[HEXDUMP] Address: %p len: 0x%lX (%ld)", src, len, len);
    while (len > 0) {
        uint32_t linesize = cols > len ? len : cols;
        printf("\n[%p] 0x%04x: ", src, (int)(src - (const char*)mem));
        for (uint32_t i = 0; i < linesize; i++) {
            printf("%02x ", *(src + i));
        }
        printf("  ");
        for (uint32_t i = linesize; i < cols; i++) {
            printf("   ");
        }
        for (uint32_t i = 0; i < linesize; i++) {
            unsigned char c = *(src + i);
            putc(isprint(c) ? c : '.', stdout);
        }
        src += linesize;
        len -= linesize;
    }
    printf("\n");
}

