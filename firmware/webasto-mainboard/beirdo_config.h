#ifndef __beirdo_config_h_
#define __beirdo_config_h_

#ifndef RASPBERRY_PI_PICO
#define RASPBERRY_PI_PICO
#endif

#define USE_MUTEX
#undef DISABLE_LOGGING
#undef HEXDUMP_TX

#define USE_I2C
#define USE_SPI

#endif
