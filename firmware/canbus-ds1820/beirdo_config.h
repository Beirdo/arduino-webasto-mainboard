#ifndef __beirdo_config_h_
#define __beirdo_config_h_

#include "hal_conf_extra.h"

#ifndef STM32F0xx
#define STM32F0xx
#endif

#undef USE_MUTEX
#undef DISABLE_LOGGING

#ifdef HAL_I2C_MODULE_DISABLED
#undef USE_I2C
#else
#define USE_I2C
#endif

#ifdef HAL_SPI_MODULE_DISABLED
#undef USE_SPI
#else
#define USE_SPI
#endif

#endif