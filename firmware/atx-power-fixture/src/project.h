#ifndef __project_h_
#define __project_h_

#include <Arduino.h>
#include <Beirdo-Utilities.h>

// unused 0 // PA4
// unused 1 // PA5
// unused 2 // PA6
#define PIN_ONBOARD_LED     3   // PA7
#define PIN_RXD             4   // PB3
#define PIN_TXD             5   // PB2
#define PIN_SDA             6   // PB1
#define PIN_SCL             7   // PB0
#define PIN_IGNITION_SWITCH 8   // PA1
#define PIN_IGNITION_OUT    9   // PA2
#define PIN_RESET           10  // PA3
#define PIN_UPDI            11  // PA0

#define I2C_ADDR_INA219     0x4F
#define I2C_ADDR_OLED       0x3C    // or 0x3D

#endif
