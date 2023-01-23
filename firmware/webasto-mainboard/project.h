#ifndef __project_h_
#define __project_h_

// I2C definitions
#define PIN_I2C0_SDA 8
#define PIN_I2C0_SCL 9
#define I2C0_CLK 400000


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

inline int clamp(int value, int minval, int maxval)
{
  return max(min(value, maxval), minval);
}

#define HIBYTE(x)   ((uint8_t)(((int)(x) >> 8) & 0xFF))
#define LOBYTE(x)   ((uint8_t)(((int)(x) & 0xFF)))

#endif
