#ifndef __project_h_
#define __project_h_

#include <TCA9534-GPIO.h>
#include <Beirdo-Utilities.h>

// I2C definitions
#define I2C0_CLK 400000

// I2C Addresses
#define I2C_ADDR_OLED     0x3C  // 3D for 128x64, 3C for 128x32
#define I2C_ADDR_TCA9534  0x27

// GPIO map on the sensor boards
#define PCA9501_WRITE_EN  0x80
#define PCA9501_A2        0x40
#define PCA9501_A1        0x20
#define PCA9501_A0        0x10
#define PCA9501_VINP      0x02
#define PCA9501_VINN      0x01

// Serial1 -> Console
#ifdef PIN_SERIAL1_TX
#undef PIN_SERIAL1_TX
#endif

#ifdef PIN_SERIAL1_RX
#undef PIN_SERIAL1_RX
#endif

// Serial2 -> KLine
#ifdef PIN_SERIAL2_TX
#undef PIN_SERIAL2_TX
#endif

#ifdef PIN_SERIAL2_RX
#undef PIN_SERIAL2_RX
#endif

#ifdef PIN_SPI0_MISO
#undef PIN_SPI0_MISO
#endif

#ifdef PIN_SPI0_SS
#undef PIN_SPI0_SS
#endif

#ifdef PIN_SPI0_SCK
#undef PIN_SPI0_SCK
#endif

#ifdef PIN_SPI0_MOSI
#undef PIN_SPI0_MOSI
#endif



// outputs
#define PIN_SERIAL1_TX        0
#define PIN_SERIAL1_RX        1
#define PIN_KLINE_EN          3
#define PIN_SERIAL2_TX        4
#define PIN_SERIAL2_RX        5
#define PIN_GLOW_PLUG_OUT_EN  6
#define PIN_GLOW_PLUG_IN_EN   7
#define PIN_I2C0_SDA          8
#define PIN_I2C0_SCL          9
#define PIN_CIRCULATION_PUMP  10
#define PIN_COMBUSTION_FAN    11
#define PIN_GLOW_PLUG_OUT     12
#define PIN_FUEL_PUMP         13
#define PIN_ALERT_BUZZER      15
#define PIN_SPI0_MISO         16
#define PIN_SPI0_SS           17
#define PIN_SPI0_SCK          18
#define PIN_SPI0_MOSI         19
#define PIN_CAN_INT           10
#define PIN_BOARD_SENSE       22  // LOW = mainboard there, HIGH = just the Pico
#define PIN_RESERVED_WIFI_ON  23
#define PIN_RESERVED_WIFI_DAT 24
#define PIN_RESERVED_WIFI_CS  25

// On the TCA9534 GPIO Expander
#define PIN_CAN_SOF           0
#define PIN_POWER_LED         4
#define PIN_OPERATING_LED     5
#define PIN_FLAME_LED         6
#define PIN_CAN_EN            7



#define SUPPLEMENTAL_MIN_TEMP   -2000   // -20C
#define SUPPLEMENTAL_MAX_TEMP   1000    // 10C

#define BATTERY_LOW_THRESHOLD   10000   // 10V

#define MAX_FLAMEOUT_COUNT      2

#define FLAME_DETECT_THRESHOLD  800     // 800 mOhm... just guessing for now.

#define EXHAUST_PURGE_THRESHOLD 12500   // 125C
#define EXHAUST_IDLE_THRESHOLD  20000   // 200C
#define EXHAUST_MAX_TEMP        35000   // 350C - too hot!!
#define EXHAUST_TEMP_RISE       1500    // 15C

#define INTERNAL_MAX_TEMP       9500    // 95C - hot enough

#define WINTER_TEMP_THRESHOLD   1000    // 10C

#define PRIMING_LOW_THRESHOLD  -1000    // -10C
#define PRIMING_HIGH_THRESHOLD  2000    //  20C

#define COOLANT_COLD_THRESHOLD  4000    // 40C
#define COOLANT_MIN_THRESHOLD   5500    // 55C
#define COOLANT_TARGET_TEMP     6500    // 65C
#define COOLANT_IDLE_THRESHOLD  7500    // 75C
#define COOLANT_MAX_THRESHOLD   8500    // 85C - too hot!!!

#define THROTTLE_HIGH_FUEL(x)   (double)((x) <= WINTER_TEMP_THRESHOLD ? 1.8 : 1.6)
#define THROTTLE_HIGH_FAN       90

#define THROTTLE_STEADY_FUEL    (double)(1.3)
#define THROTTLE_STEADY_FAN     65

#define THROTTLE_LOW_FUEL       (double)(0.83)
#define THROTTLE_LOW_FAN        55

#define THROTTLE_IDLE_FUEL      (double)(0.6)
#define THROTTLE_IDLE_FAN       30

#define START_FUEL(x)           (double)((x) <= WINTER_TEMP_THRESHOLD ? 1.2 : 1.0)
#define START_FAN               40

#define PURGE_FAN               80

extern bool mainboardDetected;
extern TCA9534 tca9534;


#endif
