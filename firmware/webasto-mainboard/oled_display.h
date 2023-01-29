#ifndef __oled_display_h_
#define __oled_display_h_

#include <Adafruit_SSD1306.h>

#include "display.h"

class OLEDDisplay : public Display {
  public:
    OLEDDisplay(uint8_t i2c_address, int width, int height);
    ~OLEDDisplay(void);
    void updateDisplay(void);
    void timerCallback(int timerId, int delayMs);

  protected:
    void setCursor(int x, int y);
    void write(uint16_t ch);
    void display(void);
    void clearDisplay(void);

    int _width;
    int _height;
    int _x_offset;
    bool _initialized;
    Adafruit_SSD1306 *_ssd1306;      
};

void oledTimerCallback(int timerId, int delayMs);

#endif