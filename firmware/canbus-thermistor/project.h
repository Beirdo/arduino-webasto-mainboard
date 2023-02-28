#ifndef __project_h_
#define __project_h_

#include <Beirdo-Utilities.h>

#include "internal_adc.h"

// Serial2 -> Console
#ifdef PIN_SERIAL2_TX
#undef PIN_SERIAL2_TX
#endif

#ifdef PIN_SERIAL2_RX
#undef PIN_SERIAL2_RX
#endif

#ifdef PIN_SPI1_MISO
#undef PIN_SPI1_MISO
#endif

#ifdef PIN_SPI1_SS
#undef PIN_SPI1_SS
#endif

#ifdef PIN_SPI1_SCK
#undef PIN_SPI1_SCK
#endif

#ifdef PIN_SPI1_MOSI
#undef PIN_SPI1_MOSI
#endif



#define PIN_VTHERM            0   // PA_0
#define PIN_VBAT_MON          1   // PA_1
#define PIN_USART2_TX         2   // PA_2
#define PIN_USART2_RX         3   // PA_3
#define PIN_SPI1_SS           4   // PA_4
#define PIN_SPI1_SCK          5   // PA_5
#define PIN_SPI1_MISO         6   // PA_6
#define PIN_SPI1_MOSI         7   // PA_7
#define PIN_SWDIO             10  // PA_13
#define PIN_SWDCLK            11  // PA_14
#define PIN_CAN_EN            12  // PB_1
#define PIN_BOOTROM_SEL       13  // PB_8
#define PIN_I2C1_SDA          14  // PF_0
#define PIN_I2C1_SCL          15  // PF_1
#define PIN_CAN_RX            16  // PA_11_R
#define PIN_USB_DN            16  // PA_11_R
#define PIN_CAN_TX            17  // PA_12_R
#define PIN_USB_DP            17  // PA_12_R


extern InternalADCSensor coolantTempSensor;
extern InternalADCSensor batteryVoltageSensor;

#endif
