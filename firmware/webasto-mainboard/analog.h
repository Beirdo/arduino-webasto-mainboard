#ifndef __analog_h
#define __analog_h

#include <pico.h>
#include "ds2482.h"

void init_analog(void);
void update_analog(void);

extern DS2482Source externalTempSensor;

#endif
