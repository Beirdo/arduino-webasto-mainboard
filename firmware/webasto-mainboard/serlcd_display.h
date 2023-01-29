#ifndef __serlcd_display_h_
#define __serlcd_display_h_

#include <SerLCD.h>
#include "display.h"

class SerLCDDisplay : public Display {
  public:
    SerLCDDisplay(uint8_t i2c_address, int columns, int rows);
    void updateDisplay(void);

  protected:
    void setCursor(int x, int y);
    void write(uint16_t ch);
    void display(void);
    void clearDisplay(void);
    
    SerLCD _lcd;      
};


#endif