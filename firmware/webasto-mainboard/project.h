#ifndef __project_h_
#define __project_h_

// I2C definitions
#define PIN_I2C0_SDA 8
#define PIN_I2C0_SCL 9
#define I2C0_CLK 400000


// I2C Addresses
#define I2C_ADDR_SERLCD   0x7F
#define I2C_ADDR_OLED     0x3C  // 3D for 128x64, 3C for 128x32

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
#define PIN_SERIAL1_TX  0

#ifdef PIN_SERIAL1_RX
#undef PIN_SERIAL1_RX
#endif
#define PIN_SERIAL1_RX  1

#ifdef PIN_SERIAL1_CTS
#undef PIN_SERIAL1_CTS
#endif
#define PIN_SERIAL1_CTS 18

#ifdef PIN_SERIAL1_RTS
#undef PIN_SERIAL1_RTS
#endif
#define PIN_SERIAL1_RTS 19


// Serial2 -> KLine
#ifdef PIN_SERIAL2_TX
#undef PIN_SERIAL2_TX
#endif
#define PIN_SERIAL2_TX  4

#ifdef PIN_SERIAL2_RX
#undef PIN_SERIAL2_RX
#endif
#define PIN_SERIAL2_RX  5


// inputs
#define PIN_START_RUN         2

// outputs
#define PIN_KLINE_EN          3
#define PIN_GLOW_PLUG_OUT_EN  6
#define PIN_GLOW_PLUG_IN_EN   7
#define PIN_CIRCULATION_PUMP  10
#define PIN_COMBUSTION_FAN    11
#define PIN_GLOW_PLUG_OUT     12
#define PIN_FUEL_PUMP         13
#define PIN_VEHICLE_FAN_RELAY 14
#define PIN_ALERT_BUZZER      15
#define PIN_EMERGENCY_STOP    16
#define PIN_USE_USB           17
#define PIN_BOARD_SENSE       22  // LOW = mainboard there, HIGH = just the Pico
#define PIN_RESERVED_WIFI_ON  23
#define PIN_RESERVED_WIFI_DAT 24
#define PIN_RESERVED_WIFI_CS  25
#define PIN_FLAME_LED         26
#define PIN_OPERATING_LED     27
#define PIN_ONBOARD_LED       32

#define HI_BYTE(x)    ((uint8_t)(((int)(x) >> 8) & 0xFF))
#define LO_BYTE(x)    ((uint8_t)(((int)(x) & 0xFF)))

#define HI_NIBBLE(x)  ((uint8_t)(((int)(x) >> 4) & 0x0F))
#define LO_NIBBLE(x)  ((uint8_t)(((int)(x) & 0x0F)))

#define BIT(x) ((uint32_t)(1 << x))

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

extern void hexdump(const void* mem, uint32_t len, uint8_t cols);

template <typename T>
inline T clamp(T value, T minval, T maxval)
{
  return max(min(value, maxval), minval);
}

template <typename T>
inline T map(T x, T in_min, T in_max, T out_min, T out_max)
{
  // the perfect map fonction, with constraining and float handling
  x = clamp<T>(x, in_min, in_max);
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


#endif
