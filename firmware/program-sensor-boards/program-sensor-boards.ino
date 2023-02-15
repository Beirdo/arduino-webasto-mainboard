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
  output->printf("%20d: ", millis());
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

void hexdump(const void* mem, uint32_t len, uint8_t cols) 
{
  const char* src = (const char*)mem;
  static char line[128];
  char *ch = line;
  int written;  
      
  Log.info("[HEXDUMP] Address: %X len: %X (%d)", src, len, len);
  while (len > 0) {
    ch = line;
    memset(ch, 0x00, 128);

    uint32_t linesize = cols > len ? len : cols;
    written = sprintf(ch, "[%X] 0x%04x: ", src, (int)(src - (const char*)mem));
    ch += written;
    for (uint32_t i = 0; i < linesize; i++) {
        written = sprintf(ch, "%02x ", *(src + i));
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


