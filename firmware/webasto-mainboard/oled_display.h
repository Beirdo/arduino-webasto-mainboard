#ifndef __oled_display_h_
#define __oled_display_h_

#include <Adafruit_SSD1306.h>

#include "display.h"

class OLEDDisplay : public Display {
  public:
    OLEDDisplay(uint8_t i2c_address, int width, int height);

  protected:
    void setCursor(int x, int y);
    void write(uint16_t ch);
    void display(void);

    int _width;
    int _height;
    int _x_offset;
    Adafruit_SSD1306 *_ssd1306;      
};

#endif