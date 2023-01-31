#ifndef __project_h_
#define __project_h_

#define PIN_LOCAL_TX        0
#define PIN_LOCAL_RX        1
#define PIN_LOCAL_RTS       2
#define PIN_LOCAL_CTS       3
#define PIN_I2C_SDA         8
#define PIN_I2C_SCL         9
#define PIN_IGNITION_OUT    16
#define PIN_IGNITION_SWITCH 17
#define PIN_BOARD_SENSE     18
#define PIN_ONBOARD_LED     25

#define I2C_ADDR_INA219     0x4F
#define I2C_ADDR_OLED       0x3C    // or 0x3D

#define HIBYTE(x)     ((uint8_t)(((int)(x) >> 8) & 0xFF))
#define LOBYTE(x)     ((uint8_t)(((int)(x) & 0xFF)))

#define HI_NIBBLE(x)  ((uint8_t)(((int)(x) >> 4) & 0x0F))
#define LO_NIBBLE(x)  ((uint8_t)(((int)(x) & 0x0F)))

inline int clamp(int value, int minval, int maxval)
{
  return max(min(value, maxval), minval);
}

#endif