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
#define PIN_ONBOARD_LED     32

#define I2C_ADDR_INA219     0x4F
#define I2C_ADDR_OLED       0x3C    // or 0x3D

#define HI_BYTE(x)     ((uint8_t)(((int)(x) >> 8) & 0xFF))
#define LO_BYTE(x)     ((uint8_t)(((int)(x) & 0xFF)))

#define HI_NIBBLE(x)  ((uint8_t)(((int)(x) >> 4) & 0x0F))
#define LO_NIBBLE(x)  ((uint8_t)(((int)(x) & 0x0F)))

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
