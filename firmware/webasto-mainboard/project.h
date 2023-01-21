#ifndef __project_h_
#define __project_h_

#define PIN_I2C0_SDA 8
#define PIN_I2C0_SCL 9
#define I2C0_CLK 400000

#define PCA9501_WRITE_EN  0x80
#define PCA9501_A2        0x40
#define PCA9501_A1        0x20
#define PCA9501_A0        0x10
#define PCA9501_VINP      0x02
#define PCA9501_VINN      0x01

// goes into glow_plug later
#define GLOW_PLUG_IN_EN_PIN 7
#define GLOW_PLUG_OUT_EN_PIN 6

#endif